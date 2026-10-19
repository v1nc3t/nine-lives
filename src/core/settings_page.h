#pragma once

#include <M5Cardputer.h>

// Page 0. A list of rows: back ('<') first, then brightness.
// Up/down (fn + ; and .) move between rows, left/right (fn + , and /) change the value, Backspace always goes back.
void settingsPageDraw();
bool settingsPageHandleKey(const Keyboard_Class::KeysState &keys);
