#pragma once

#include <M5Cardputer.h>

// Page 0. For now only the back row ('<'); Enter or Backspace returns to the main screen (see pageHandleBack).
void settingsPageDraw();
bool settingsPageHandleKey(const Keyboard_Class::KeysState &keys);
