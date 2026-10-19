#include "status_bar.h"

#include <M5Cardputer.h>

#include "clock.h"

static constexpr uint32_t CHECK_MS = 1000;
static constexpr int LOW_LEVEL = 15; // percent; at or below this (and not charging) the level turns red

static char drawnClock[6] = ""; // clock text shown right now
static int drawnLevel = -1;   // battery level shown right now
static bool drawnLow = false; // whether it was drawn as a warning

static bool isLow(int level) {
    return level <= LOW_LEVEL && M5Cardputer.Power.isCharging() != m5::Power_Class::is_charging;
}

static void draw(int level, bool low) {
    auto &lcd = M5Cardputer.Display;
    int w = lcd.width();

    lcd.fillRect(0, 0, w, STATUS_BAR_H, TFT_BLACK);
    lcd.setTextDatum(middle_left);
    lcd.setTextSize(1.2);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.drawString(clockText(), 4, STATUS_BAR_H / 2);
    strcpy(drawnClock, clockText());

    lcd.setTextDatum(middle_right);
    if (low) lcd.setTextColor(TFT_RED, TFT_BLACK);
    lcd.drawString((low ? "LOW " : "") + String(level) + "%", w - 4, STATUS_BAR_H / 2);
    drawnLevel = level;
    drawnLow = low;
}

void statusBarDraw() {
    int level = M5Cardputer.Power.getBatteryLevel();
    draw(level, isLow(level));
}

void statusBarUpdate() {
    static uint32_t last = 0;
    if (millis() - last < CHECK_MS) return;
    last = millis();

    int level = M5Cardputer.Power.getBatteryLevel();
    bool low = isLow(level);
    if (level != drawnLevel || low != drawnLow || strcmp(clockText(), drawnClock) != 0) draw(level, low);
}
