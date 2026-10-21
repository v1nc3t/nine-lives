#pragma once

#include <M5Cardputer.h>

// Page 0. A list of rows: back ('<') first, then brightness, wifi, connect, time, time zone, sleep time, deep sleep time and power saving.
// The list scrolls when the selection moves past the screen.
// Up/down (fn + ; and .) move between rows, left/right (fn + , and /) change the value, Backspace and ESC always go back.
// Enter on the wifi, power saving or sound row switches it on or off, the play sound row picks (left/right) the sound that main.cpp plays on every key press and BtnGO while sound is on (see sound.h), on the connect row it opens the Wi-Fi popup (see wifi_popup.h, which main.cpp runs).
// Enter on the sd card row (shows the card's size, or "no card") opens its files as a page of their own (see sd_browser.h).
// Enter on the time row opens a popup: type HH:MM on the number keys, fn + , and / move the cursor, fn + ; and . (or BtnGO, +1 only) change the digit, Enter saves, Backspace or ESC cancels.
void settingsPageDraw();
bool settingsPageHandleKey(const Keyboard_Class::KeysState &keys);
bool settingsPageHandleButton(); // BtnGO
bool settingsPageUpdate();       // every loop()
