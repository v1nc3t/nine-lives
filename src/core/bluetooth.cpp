#include "bluetooth.h"

#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <M5Cardputer.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>

#include "settings.h"

// Advertising intervals in units of 0.625 ms: quick while a phone may pair (it is looking), slow while waiting for the remembered one
static constexpr uint16_t ADV_FAST_MIN = 0xA0;  // 100 ms
static constexpr uint16_t ADV_FAST_MAX = 0xF0;  // 150 ms
static constexpr uint16_t ADV_SLOW_MIN = 0x640; // 1 s
static constexpr uint16_t ADV_SLOW_MAX = 0x7D0; // 1.25 s

// The device presents itself as a HID accessory (a media-key remote with no keys yet): iOS lists those in Settings > Bluetooth,
// and reconnects to a paired one by itself. Plain BLE devices only show up in apps like nRF Connect. It has to match the input report below.
static const uint8_t REPORT_MAP[] = {
    0x05, 0x0C,       // usage page: consumer
    0x09, 0x01,       // usage: consumer control
    0xA1, 0x01,       // collection (application)
    0x85, 0x01,       //   report ID 1
    0x15, 0x00,       //   logical min 0
    0x26, 0xFF, 0x03, //   logical max 0x3FF
    0x19, 0x00,       //   usage min 0
    0x2A, 0xFF, 0x03, //   usage max 0x3FF
    0x75, 0x10,       //   report size 16 bits
    0x95, 0x01,       //   report count 1
    0x81, 0x00,       //   input (array)
    0xC0,             // end collection
};

static constexpr uint16_t CURRENT_TIME = 0x2A2B; // characteristic of the Current Time service, which an iPhone offers over its link

static volatile bool pairingOpen = false;
static volatile bool linked = false; // the phone is connected and encrypted; set from the BLE task

// The phone's clock, read from the BLE task and handed over to bluetoothTime() in the main loop (the RTC chip is not for two tasks)
static esp_gatt_if_t clientIf = ESP_GATT_IF_NONE; // the client that reads it; none until the stack has registered it
static esp_bd_addr_t phoneAddress;
static esp_ble_addr_type_t phoneAddressType;
static bool timeAsked = false; // once per connection
static time_t phoneTime;
static volatile bool phoneTimeReady = false;

// The stack answers the bond calls only while it runs
static bool running() { return esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED; }
static int bondCount() { return running() ? esp_ble_get_bond_device_num() : 0; }

// Calls action(address) for each remembered phone (normally just one)
template <typename F> static void bondsEach(F action) {
    int count = bondCount();
    if (count <= 0) return;
    auto *list = (esp_ble_bond_dev_t *)malloc(count * sizeof(esp_ble_bond_dev_t));
    if (list && esp_ble_get_bond_device_list(&count, list) == ESP_OK) {
        for (int i = 0; i < count; i++) action(list[i].bd_addr);
    }
    free(list);
}

// (Re)starts advertising at the pace that fits: the stack stops it when a phone connects, and the pace changes with the pairing window
static void advertise() {
    bool fast = pairingOpen && bondCount() == 0;
    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->setMinInterval(fast ? ADV_FAST_MIN : ADV_SLOW_MIN);
    advertising->setMaxInterval(fast ? ADV_FAST_MAX : ADV_SLOW_MAX);
    advertising->stop(); // fails harmlessly when it is not running
    advertising->start();
}

// Asks the phone for its time. The phone made the connection, so opening a client on it only attaches to it; the answer comes in clientEvent().
// An iPhone has the Current Time service, an Android phone has none, so there nothing comes back.
static void askTime() {
    if (clientIf == ESP_GATT_IF_NONE || timeAsked) return;
    timeAsked = true;
    esp_ble_gattc_open(clientIf, phoneAddress, phoneAddressType, true);
}

// Events of that client. The library's own client handler only serves its BLEClient, which is not used, so this one takes its place.
static void clientEvent(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    if (event == ESP_GATTC_REG_EVT) {
        clientIf = param->reg.status == ESP_GATT_OK ? gattc_if : ESP_GATT_IF_NONE;
    } else if (event == ESP_GATTC_DIS_SRVC_CMPL_EVT && param->dis_srvc_cmpl.status == ESP_GATT_OK) { // the phone's services are known
        esp_bt_uuid_t uuid = {};
        uuid.len = ESP_UUID_LEN_16;
        uuid.uuid.uuid16 = CURRENT_TIME;
        esp_gattc_char_elem_t current;
        uint16_t count = 1;
        uint16_t conn = param->dis_srvc_cmpl.conn_id;
        if (esp_ble_gattc_get_char_by_uuid(gattc_if, conn, 1, 0xFFFF, uuid, &current, &count) == ESP_GATT_OK && count == 1) {
            esp_ble_gattc_read_char(gattc_if, conn, current.char_handle, ESP_GATT_AUTH_REQ_NONE);
        }
    } else if (event == ESP_GATTC_READ_CHAR_EVT && param->read.status == ESP_GATT_OK && param->read.value_len >= 7) {
        // Local date and time: year (2 bytes, little endian), month, day, hours, minutes, seconds; year or month 0 means the phone does not know
        const uint8_t *v = param->read.value;
        int year = v[0] | v[1] << 8;
        if (year < 2020 || v[2] < 1 || v[2] > 12 || v[3] < 1) return;
        tm local = {};
        local.tm_year = year - 1900;
        local.tm_mon = v[2] - 1;
        local.tm_mday = v[3];
        local.tm_hour = v[4];
        local.tm_min = v[5];
        local.tm_sec = v[6];
        phoneTime = mktime(&local); // as if it were UTC, as no time zone is configured
        phoneTimeReady = true;
    }
}

class Link : public BLEServerCallbacks, public BLESecurityCallbacks {
    void onConnect(BLEServer *, esp_ble_gatts_cb_param_t *param) override {
        memcpy(phoneAddress, param->connect.remote_bda, sizeof(phoneAddress));
        phoneAddressType = param->connect.ble_addr_type;
        timeAsked = false;
        // Asks the phone to pair (a new one) or to encrypt with its saved key (the remembered one)
        esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_NO_MITM);
    }
    void onDisconnect(BLEServer *) override {
        linked = false;
        advertise(); // advertising stops with a connection: wait for the phone again
    }
    bool onSecurityRequest() override { return pairingOpen && bondCount() == 0; }
    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth) override {
        linked = auth.success;
        if (auth.success) askTime();
        else esp_ble_gap_disconnect(auth.bd_addr); // refused, or the phone gave up
    }
    uint32_t onPassKeyRequest() override { return 0; }
    void onPassKeyNotify(uint32_t) override {}
    bool onConfirmPIN(uint32_t) override { return false; }
};
static Link phoneLink;

static void radioOn() {
    if (BLEDevice::getInitialized()) return;
    BLEDevice::init(BLUETOOTH_NAME);
    BLEDevice::setSecurityCallbacks(&phoneLink);
    esp_ble_gattc_register_callback(clientEvent);
    esp_ble_gattc_app_register(0);
    BLEServer *server = BLEDevice::createServer();
    server->setCallbacks(&phoneLink);

    // shortcut: every switch-on leaks the previous server and services (a few KB, the library can't free them), fine for a few switches per boot
    BLEHIDDevice *hid = new BLEHIDDevice(server);
    hid->pnp(0x02, 0xE502, 0xA111, 0x0210);
    hid->hidInfo(0x00, 0x01);
    hid->reportMap((uint8_t *)REPORT_MAP, sizeof(REPORT_MAP));
    hid->inputReport(1);
    hid->startServices();
    hid->setBatteryLevel(constrain(M5Cardputer.Power.getBatteryLevel(), 0, 100));

    // Pair and remember (bond), keys exchanged both ways; no passkey: the IO capability init() sets is "none"
    uint8_t auth = ESP_LE_AUTH_REQ_SC_BOND;
    uint8_t keys = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth, sizeof(auth));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &keys, sizeof(keys));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &keys, sizeof(keys));

    BLEAdvertisementData advert;
    advert.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
    advert.setAppearance(HID_KEYBOARD);
    advert.setCompleteServices(hid->hidService()->getUUID());
    advert.setName(BLUETOOTH_NAME);
    BLEDevice::getAdvertising()->setAdvertisementData(advert);
    BLEDevice::getAdvertising()->setScanResponse(false); // the name is in the advert itself
    advertise();
}

void bluetoothRadioOff() {
    BLEDevice::deinit(false); // false: keep the memory, so the radio can come back
    linked = false;
    clientIf = ESP_GATT_IF_NONE;
}

bool bluetoothEnabled() { return bluetoothOn() && !powerSaving(); }
bool bluetoothRunning() { return running(); }

void bluetoothApply() {
    if (bluetoothEnabled()) radioOn();
    else bluetoothRadioOff();
}

void bluetoothSetEnabled(bool on) {
    if (powerSaving() || on == bluetoothEnabled()) return;
    bluetoothOnSet(on);
    bluetoothApply();
}

void bluetoothPairing(bool open) {
    pairingOpen = open;
    if (BLEDevice::getInitialized()) advertise();
}

void bluetoothForget() {
    bondsEach([](uint8_t *address) { esp_ble_remove_bond_device(address); });
}

bool bluetoothTime(time_t *local) {
    if (!phoneTimeReady) return false;
    phoneTimeReady = false;
    *local = phoneTime;
    return true;
}

BluetoothStatus bluetoothStatus() {
    if (bondCount() <= 0) return BLUETOOTH_NONE;
    return linked ? BLUETOOTH_CONNECTED : BLUETOOTH_PAIRED;
}

const char *bluetoothDevice() {
    static char text[18];
    text[0] = 0;
    bondsEach([](uint8_t *a) {
        if (!text[0]) snprintf(text, sizeof(text), "%02X:%02X:%02X:%02X:%02X:%02X", a[0], a[1], a[2], a[3], a[4], a[5]);
    });
    return text;
}
