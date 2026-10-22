#pragma once

#include <M5Cardputer.h>

// Popup for the Bluetooth phone link, opened from the pair row in settings or with '=' on the home screen.
// Like the Wi-Fi popup (see wifi_popup.h) it sits on top of whatever is open and takes over the keys until it closes (main.cpp runs it):
//  - opening it switches Bluetooth on and opens the pairing window: pair "cardputer" from the phone's Bluetooth settings
//  - it shows whether a phone is remembered and whether it is connected; the phone reconnects by itself later
//  - Enter or BtnGO forgets the remembered phone (so another one can pair); Backspace or ESC closes
void bluetoothPopupOpen();
bool bluetoothPopupIsOpen();
// The handlers return true if a redraw is needed; if the popup has just closed, that is the whole screen, not only the popup.
void bluetoothPopupDraw();             // paints just the popup, on top of the screen
bool bluetoothPopupHandleKey(const Keyboard_Class::KeysState &keys);
bool bluetoothPopupHandleButton();     // BtnGO
bool bluetoothPopupUpdate();           // call every loop() while open: picks up a phone pairing, connecting or leaving
