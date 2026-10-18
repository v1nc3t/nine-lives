#include "status_bar.h"

#include <M5Cardputer.h>

static constexpr uint32_t CHECK_MS = 5000;

static int drawnLevel = -1; // battery level shown right now

static void draw(int level) {
    auto &lcd = M5Cardputer.Display;
    int w = lcd.width();

    lcd.fillRect(0, 0, w, STATUS_BAR_H, TFT_BLACK);
    lcd.setTextDatum(middle_left);
    lcd.setTextSize(1.2);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.drawString("nine lives", 4, STATUS_BAR_H / 2);

    lcd.setTextDatum(middle_right);
    lcd.drawString(String(level) + "%", w - 4, STATUS_BAR_H / 2);
    drawnLevel = level;
}

void statusBarDraw() {
    draw(M5Cardputer.Power.getBatteryLevel());
}

void statusBarUpdate() {
    static uint32_t last = 0;
    if (millis() - last < CHECK_MS) return;
    last = millis();

    int level = M5Cardputer.Power.getBatteryLevel();
    if (level != drawnLevel) draw(level);
}
