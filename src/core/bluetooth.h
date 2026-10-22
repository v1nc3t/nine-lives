#pragma once

// Bluetooth LE link to a phone: the on/off switch (kept in settings) and one remembered phone.
// The Cardputer waits and the phone connects: while Bluetooth is on it advertises as BLUETOOTH_NAME (as a HID accessory, so the phone lists it).
// A phone that paired while the pairing window was open is remembered (the BLE stack keeps its keys in flash),
// and it reconnects by itself whenever it is back in range, also after a reboot.
// Only one phone is remembered: pairing another one needs the first forgotten.

static constexpr const char *BLUETOOTH_NAME = "cardputer"; // what the phone shows

bool bluetoothEnabled();           // the setting, and always false while power saving is on
bool bluetoothRunning();           // the radio really started (false right after switching on means it failed)
void bluetoothSetEnabled(bool on); // switches the radio and saves; does nothing while power saving is on
void bluetoothApply();             // makes the radio match bluetoothEnabled(): call in setup() after settingsLoad(), and after power saving was switched
void bluetoothRadioOff();          // turns the radio off without touching the setting, for deep sleep

// While the pairing window is open a new phone may pair, if none is remembered. The popup (see bluetooth_popup.h) keeps it open.
void bluetoothPairing(bool open);
void bluetoothForget();            // forgets the remembered phone and drops its connection (the status changes a moment later)

enum BluetoothStatus { BLUETOOTH_NONE, BLUETOOTH_PAIRED, BLUETOOTH_CONNECTED }; // no phone remembered / remembered, not connected / connected
BluetoothStatus bluetoothStatus();
const char *bluetoothDevice();     // address of the remembered phone, e.g. "A4:83:E7:12:34:56", or "" (static buffer)
