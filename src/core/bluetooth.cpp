#include "bluetooth.h"

#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <M5Cardputer.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>

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

static volatile bool pairingOpen = false;
static volatile bool linked = false; // the phone is connected and encrypted; set from the BLE task

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

class Link : public BLEServerCallbacks, public BLESecurityCallbacks {
    void onConnect(BLEServer *, esp_ble_gatts_cb_param_t *param) override {
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
        if (!auth.success) esp_ble_gap_disconnect(auth.bd_addr); // refused, or the phone gave up
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
