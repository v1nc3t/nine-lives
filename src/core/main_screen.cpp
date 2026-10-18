#include "main_screen.h"

#include <M5Cardputer.h>

#include "page_bar.h"
#include "status_bar.h"

static constexpr int ARROW_INSET = 12; // distance of the arrows' centres from the left/right screen edge

void mainScreenDraw(char label) {
    auto &lcd = M5Cardputer.Display;
    int w = lcd.width();
    int h = lcd.height() - STATUS_BAR_H - PAGE_BAR_H;

    lcd.fillRect(0, STATUS_BAR_H, w, h, TFT_BLACK);
    lcd.drawRect(0, STATUS_BAR_H, w, h, TFT_GREEN);

    // Page arrows, inset from the frame so they don't touch the edge
    lcd.setTextDatum(middle_center);
    lcd.setTextSize(2);
    lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd.drawString("<", ARROW_INSET, STATUS_BAR_H + h / 2);
    lcd.drawString(">", w - ARROW_INSET, STATUS_BAR_H + h / 2);

    if (label) {
        lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        lcd.setTextSize(6);
        lcd.drawString(String(label), w / 2, STATUS_BAR_H + h / 2);
    }
}
