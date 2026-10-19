#pragma once

#include <M5Cardputer.h>

// Page 0. A list of rows: back ('<') first, then brightness, sleep time, deep sleep time, time and time zone.
// Up/down (fn + ; and .) move between rows, left/right (fn + , and /) change the value, Backspace and ESC always go back.
// Enter on the time row opens a popup: type HH:MM on the number keys, fn + , and / move the cursor, fn + ; and . (or BtnGO, +1 only) change the digit, Enter saves, Backspace or ESC cancels.
void settingsPageDraw();
bool settingsPageHandleKey(const Keyboard_Class::KeysState &keys);
bool settingsPageHandleButton(); // BtnGO
