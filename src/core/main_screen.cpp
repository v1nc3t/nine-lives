#include "main_screen.h"

#include <M5Cardputer.h>

#include "lgfx/v1/misc/enum.hpp"
#include "page.h"

static constexpr int ARROW_INSET = 12; // distance of the arrows' centres from the left/right screen edge

void mainScreenDraw(const char *name, const char *description) {
    auto &lcd = M5Cardputer.Display;
    int w = lcd.width();
    int h = lcd.height() - STATUS_BAR_H - PAGE_BAR_H;

    pageFrameDraw(true);

    // Page arrows, inset from the frame so they don't touch the edge
    lcd.setTextDatum(middle_center);
    lcd.setTextSize(2);
    lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd.drawString("<", ARROW_INSET, STATUS_BAR_H + h / 2);
    lcd.drawString(">", w - ARROW_INSET, STATUS_BAR_H + h / 2);

    if (name) {
        lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        lcd.setTextSize(3);
        lcd.drawString(name, w / 2, STATUS_BAR_H + h / 2 - 10);
        lcd.setTextColor(TFT_GREEN, TFT_BLACK);
        lcd.setTextSize(1.5);
        lcd.drawString(description, w / 2, STATUS_BAR_H + h / 2 + 18);
    }
}
