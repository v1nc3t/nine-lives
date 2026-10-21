#pragma once

#include <stdint.h>

// Wi-Fi: the on/off switch (kept in settings), scanning for networks and joining one.
// A network that was joined successfully is remembered with its password (in flash, see settings.h for where the other settings live),
// the most recently joined first. While Wi-Fi is on the device rejoins the latest one by itself, also after a reboot.

// Call once in setup() after settingsLoad(): starts the radio if the setting says so.
void wifiBegin();

bool wifiEnabled();            // the setting, and always false while power saving is on
void wifiSetEnabled(bool on);  // switches the radio and saves; does nothing while power saving is on
void wifiApply();              // makes the radio match wifiEnabled(), after power saving was switched
bool wifiConnected();
const char *wifiSsid();       // name of the joined network, or "" (static buffer)

// Turns the radio off without touching the setting, for deep sleep.
void wifiRadioOff();

// While on, a connected radio wakes for fewer of the router's beacons: less power, slower replies. For sleep.
void wifiLowPower(bool on);

// Scan: wifiScanStart() turns Wi-Fi on if it was off. Poll wifiScanPoll() until it returns the number found (-1 while scanning).
// The list has one entry per network name (the strongest of them), strongest first.
struct WifiNetwork {
    char ssid[33];
    int8_t rssi;
    bool secured;
};
static constexpr int WIFI_NETWORKS_MAX = 16;
void wifiScanStart();
int wifiScanPoll();
const WifiNetwork &wifiNetwork(int index);

// Join: an open network takes an empty password. Poll wifiConnectPoll() until it is not 0.
// A successful join is remembered; a failed join with a remembered password forgets it (it is probably out of date).
void wifiConnect(const char *ssid, const char *password);
bool wifiPasswordSaved(const char *ssid);
void wifiConnectSaved(const char *ssid);  // joins with the remembered password
int wifiConnectPoll(); // 0 still trying, 1 joined, -1 failed (wrong password, out of range or timed out)
void wifiConnectCancel();
void wifiRejoin();     // after scanning or a cancelled join left the radio disconnected: back to the latest remembered network
