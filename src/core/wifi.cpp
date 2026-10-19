#include "wifi.h"

#include <Preferences.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "settings.h"

static constexpr uint32_t SCAN_RETRY_MS = 300;
static constexpr uint32_t SCAN_TIMEOUT_MS = 20000; // a scan takes about 3 s, twice that while joined to a network
static constexpr uint32_t CONNECT_TIMEOUT_MS = 15000;
static constexpr uint32_t CONNECT_SETTLE_MS = 1000; // a failure status in the first second may be left over from before

static constexpr const char *NAMESPACE = "wifi-nets";
static constexpr int KNOWN_MAX = 6;

struct Known {
    char ssid[33];
    char password[64];
};
static Known known[KNOWN_MAX]; // most recently joined first
static int knownCount = 0;
static bool knownLoaded = false;

static Known trying;           // the join in progress
static bool tryingSaved = false; // ... with a remembered password

static WifiNetwork networks[WIFI_NETWORKS_MAX];
static int networkCount = 0;
static bool scanning = false;
static uint32_t scanStarted = 0;
static uint32_t scanRetryAt = 0; // millis() to ask the driver for the scan again, 0 when it is running
static uint32_t connectStarted = 0;

static void knownLoad() {
    if (knownLoaded) return;
    knownLoaded = true;
    Preferences prefs;
    prefs.begin(NAMESPACE, true);
    for (int i = 0; i < KNOWN_MAX; i++) {
        String ssid = prefs.getString(("s" + String(i)).c_str());
        if (ssid.length() == 0 || ssid.length() >= sizeof(Known::ssid)) break;
        strcpy(known[knownCount].ssid, ssid.c_str());
        strlcpy(known[knownCount].password, prefs.getString(("p" + String(i)).c_str()).c_str(), sizeof(Known::password));
        knownCount++;
    }
    prefs.end();
}

static void knownSave() {
    Preferences prefs;
    prefs.begin(NAMESPACE, false);
    for (int i = 0; i < KNOWN_MAX; i++) {
        String ssidKey = "s" + String(i);
        String passwordKey = "p" + String(i);
        if (i < knownCount) {
            prefs.putString(ssidKey.c_str(), known[i].ssid);
            prefs.putString(passwordKey.c_str(), known[i].password);
        } else {
            prefs.remove(ssidKey.c_str());
            prefs.remove(passwordKey.c_str());
        }
    }
    prefs.end();
}

static int knownFind(const char *ssid) {
    knownLoad();
    for (int i = 0; i < knownCount; i++) {
        if (strcmp(known[i].ssid, ssid) == 0) return i;
    }
    return -1;
}

static void knownForget(const char *ssid) {
    int at = knownFind(ssid);
    if (at < 0) return;
    for (int i = at; i < knownCount - 1; i++) known[i] = known[i + 1];
    knownCount--;
    knownSave();
}

// Puts the network first, or adds it first (dropping the oldest when full)
static void knownRemember(const Known &network) {
    int at = knownFind(network.ssid);
    if (at < 0) at = min(knownCount, KNOWN_MAX - 1);
    if (at == 0 && knownCount > 0 && strcmp(known[0].password, network.password) == 0) return;
    for (int i = at; i > 0; i--) known[i] = known[i - 1];
    known[0] = network;
    knownCount = max(knownCount, at + 1);
    knownSave();
}

static void radioOn() {
    WiFi.persistent(false); // the networks are kept by this file, not also by the driver
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
}

void wifiBegin() {
    if (!wifiEnabled()) return;
    radioOn();
    wifiRejoin();
}

void wifiRejoin() {
    knownLoad();
    if (!wifiEnabled() || knownCount == 0 || WiFi.status() == WL_CONNECTED) return;
    WiFi.begin(known[0].ssid, known[0].password[0] ? known[0].password : nullptr);
}

bool wifiEnabled() { return wifiOn(); }

void wifiSetEnabled(bool on) {
    if (on == wifiEnabled()) return;
    wifiOnSet(on);
    scanning = false;
    if (on) wifiBegin();
    else WiFi.mode(WIFI_OFF);
}

void wifiRadioOff() { WiFi.mode(WIFI_OFF); }

bool wifiConnected() { return wifiEnabled() && WiFi.status() == WL_CONNECTED; }

const char *wifiSsid() {
    static char ssid[33];
    ssid[0] = 0;
    if (wifiConnected()) strlcpy(ssid, WiFi.SSID().c_str(), sizeof(ssid));
    return ssid;
}

// Asks the driver to scan. It refuses while it is busy joining a network (or still stopping one), so on a refusal
// wifiScanPoll() asks again a moment later.
static void scanBegin() {
    wifi_scan_config_t config = {};
    config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    config.scan_time.active.min = 100;
    config.scan_time.active.max = 150;
    WiFi.scanDelete();
    scanRetryAt = esp_wifi_scan_start(&config, false) == ESP_OK ? 0 : millis() + SCAN_RETRY_MS;
}

// Started with the driver's call instead of WiFi.scanNetworks(): that one gives up after 6 s, and a scan
// while joined to a network takes just over that. The Arduino library still collects the results when it is done.
void wifiScanStart() {
    if (!wifiEnabled()) { // switching on would also rejoin the saved network, which blocks scanning; that waits until the popup closes
        wifiOnSet(true);
        radioOn();
    } else if (WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect(); // stop any join in progress
    }
    networkCount = 0;
    scanning = true;
    scanStarted = millis();
    WiFi.enableSTA(true);
    scanBegin();
}

int wifiScanPoll() {
    if (!scanning) return networkCount;
    int found = WiFi.scanComplete(); // negative until the results are in
    if (found < 0 && scanRetryAt && millis() >= scanRetryAt) scanBegin();
    if (found < 0 && millis() - scanStarted < SCAN_TIMEOUT_MS) return -1;
    scanning = false;
    networkCount = 0;

    for (int i = 0; i < found; i++) { // (a scan that timed out is negative: no networks)
        String name = WiFi.SSID(i);
        if (name.length() == 0 || name.length() >= sizeof(WifiNetwork::ssid)) continue;

        int slot = -1;
        for (int n = 0; n < networkCount; n++) {
            if (name == networks[n].ssid) slot = n;
        }
        int8_t rssi = WiFi.RSSI(i);
        if (slot < 0) {
            if (networkCount == WIFI_NETWORKS_MAX) continue;
            slot = networkCount++;
        } else if (networks[slot].rssi >= rssi) {
            continue;
        }
        strcpy(networks[slot].ssid, name.c_str());
        networks[slot].rssi = rssi;
        networks[slot].secured = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();

    for (int a = 1; a < networkCount; a++) { // insertion sort, strongest first
        WifiNetwork moving = networks[a];
        int b = a - 1;
        while (b >= 0 && networks[b].rssi < moving.rssi) {
            networks[b + 1] = networks[b];
            b--;
        }
        networks[b + 1] = moving;
    }
    return networkCount;
}

const WifiNetwork &wifiNetwork(int index) { return networks[index]; }

void wifiConnect(const char *ssid, const char *password) {
    radioOn();
    strlcpy(trying.ssid, ssid, sizeof(trying.ssid));
    strlcpy(trying.password, password, sizeof(trying.password));
    tryingSaved = false;
    WiFi.begin(ssid, password[0] ? password : nullptr);
    connectStarted = millis();
}

bool wifiPasswordSaved(const char *ssid) { return knownFind(ssid) >= 0; }

void wifiConnectSaved(const char *ssid) {
    int at = knownFind(ssid);
    if (at < 0) return;
    Known network = known[at];
    wifiConnect(network.ssid, network.password);
    tryingSaved = true;
}

int wifiConnectPoll() {
    wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED) {
        knownRemember(trying);
        return 1;
    }

    uint32_t elapsed = millis() - connectStarted;
    bool failed = elapsed >= CONNECT_TIMEOUT_MS ||
                  (elapsed >= CONNECT_SETTLE_MS && (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL));
    if (!failed) return 0;
    wifiConnectCancel();
    if (tryingSaved) knownForget(trying.ssid);
    return -1;
}

void wifiConnectCancel() { WiFi.disconnect(); }
