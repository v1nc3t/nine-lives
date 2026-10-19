#pragma once

#include <cstdint>

// Color themes. The accent color replaces what used to be green: frames, the selected row, the cursor, the page bar.
// Add a theme in the THEMES table at the top of theme.cpp; it shows up in settings by itself.

int themeCount();
const char *themeName(int index);
uint32_t themeRgb(int index);          // 0xRRGGBB accent of that theme
const char *themeLedColor(int index);  // name of the LED color (see LED_COLORS in settings.cpp) the theme switches the LED to

// The selected theme (kept in settings.h's themeLevel()).
const char *themeLabel();
uint16_t themeColor();                 // its accent, ready for the screen (RGB565)
