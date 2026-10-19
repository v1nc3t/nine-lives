#pragma once

#include <M5Cardputer.h>

// Popup for joining a Wi-Fi network, opened from the connect row in settings or with '-' on the home screen.
// It sits on top of whatever is open and takes over the keys until it closes (main.cpp runs it):
//  - scans, then lists the networks found, strongest first (a padlock marks the ones with a password)
//  - up/down (fn + ; and .) move, Enter or BtnGO selects; Backspace or ESC closes
//  - the title then becomes the selected network's name, with a password field (see text_input.h for the keys):
//    Enter or BtnGO joins, fn + ` goes back to the list; open networks are joined straight away
//  - closes by itself once joined; if joining fails any key goes back to the password
void wifiPopupOpen();                  // starts a scan (turns Wi-Fi on if it was off)
bool wifiPopupIsOpen();
// The handlers return true if a redraw is needed; if the popup has just closed, that is the whole screen, not only the popup.
void wifiPopupDraw();                  // paints just the popup, on top of the screen
bool wifiPopupHandleKey(const Keyboard_Class::KeysState &keys);
bool wifiPopupHandleButton();          // BtnGO
bool wifiPopupUpdate();                // call every loop() while open: picks up scan and join results
