#include "theme.h"

#include <M5Cardputer.h>

#include "settings.h"

struct Theme {
    const char *name;     // shown in settings
    uint32_t rgb;         // 0xRRGGBB accent color
    const char *ledColor; // LED color the theme switches to: a name from LED_COLORS in settings.cpp
};

// THEMES: add a line here to add a theme. The first one is the default.
static const Theme THEMES[] = {
    {"green", 0x00FF00, "green"},
    {"cyan", 0x00FFFF, "cyan"},
    {"blue", 0x2080FF, "blue"},
    {"purple", 0x8000FF, "purple"},
    {"pink", 0xFF40A0, "pink"},
    {"red", 0xFF0000, "red"},
    {"orange", 0xFF8000, "orange"},
    {"yellow", 0xFFD000, "yellow"},
    {"white", 0xFFFFFF, "white"},
};
static constexpr int THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]);

int themeCount() { return THEME_COUNT; }

static int clampIndex(int index) { return constrain(index, 0, THEME_COUNT - 1); }

const char *themeName(int index) { return THEMES[clampIndex(index)].name; }
uint32_t themeRgb(int index) { return THEMES[clampIndex(index)].rgb; }
const char *themeLedColor(int index) { return THEMES[clampIndex(index)].ledColor; }

const char *themeLabel() { return themeName(themeLevel()); }

uint16_t themeColor() {
    uint32_t rgb = themeRgb(themeLevel());
    return M5Cardputer.Display.color565(rgb >> 16, rgb >> 8, rgb);
}
