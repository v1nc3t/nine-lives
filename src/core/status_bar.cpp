#include "status_bar.h"

#include <M5Cardputer.h>

#include "clock.h"
#include "wifi.h"

static constexpr uint32_t CHECK_MS = 1000;
static constexpr int LOW_LEVEL = 15; // percent; at or below this (and not charging) the level turns red

static char drawnClock[6] = ""; // clock text shown right now
static int drawnLevel = -1;   // battery level shown right now
static bool drawnLow = false; // whether it was drawn as a warning
static int drawnWifi = -1;    // Wi-Fi icon shown right now (see wifiState)

enum WifiState { WIFI_OFF_STATE, WIFI_SEARCHING, WIFI_JOINED };

static int wifiState() {
    if (!wifiEnabled()) return WIFI_OFF_STATE;
    return wifiConnected() ? WIFI_JOINED : WIFI_SEARCHING;
}

// 13 x 11 pixels: a dot and three arcs. Grey with a red slash when off, white while on but not joined, green when joined.
static void wifiIconDraw(int x, int y, int state) {
    auto &lcd = M5Cardputer.Display;
    uint16_t color = state == WIFI_JOINED ? TFT_GREEN : state == WIFI_SEARCHING ? TFT_WHITE : TFT_DARKGREY;
    int cx = x + 6;
    int cy = y + 10;
    lcd.fillCircle(cx, cy, 1, color);
    for (int radius = 3; radius <= 9; radius += 3) {
        for (int angle = -45; angle <= 45; angle += 3) {
            float a = angle * DEG_TO_RAD;
            lcd.drawPixel(cx + lroundf(radius * sinf(a)), cy - lroundf(radius * cosf(a)), color);
        }
    }
    if (state == WIFI_OFF_STATE) lcd.drawLine(x, y, x + 12, y + 10, TFT_RED);
}

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
    String battery = (low ? "LOW " : "") + String(level) + "%";
    lcd.drawString(battery, w - 4, STATUS_BAR_H / 2);

    drawnWifi = wifiState();
    wifiIconDraw(w - 4 - lcd.textWidth(battery) - 6 - 13, 1, drawnWifi);
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
    if (level != drawnLevel || low != drawnLow || wifiState() != drawnWifi || strcmp(clockText(), drawnClock) != 0) draw(level, low);
}
