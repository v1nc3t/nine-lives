#include "page_bar.h"

#include <M5Cardputer.h>

static const char DIGITS[] = "1234567890";
static constexpr int DIGIT_COUNT = sizeof(DIGITS) - 1;
static constexpr int BAR_H = 16;

static int selected = -1; // index into DIGITS, -1 = nothing pressed yet

static void draw() {
    auto &lcd = M5Cardputer.Display;
    int w = lcd.width();
    int mainH = lcd.height() - BAR_H;

    lcd.fillScreen(TFT_BLACK);
    lcd.drawRect(0, 0, w, mainH, TFT_GREEN);
    lcd.setTextDatum(middle_center);

    if (selected >= 0) {
        lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        lcd.setTextSize(6);
        lcd.drawString(String(DIGITS[selected]), w / 2, mainH / 2);
    }

    // Bottom bar: the GFX fonts have no bold, so the selected digit is drawn twice, 1 px apart
    lcd.setTextSize(1);
    int cell = w / DIGIT_COUNT;
    int y = mainH + BAR_H / 2;
    for (int i = 0; i < DIGIT_COUNT; i++) {
        int x = i * cell + cell / 2;
        bool on = i == selected;
        lcd.setTextColor(on ? TFT_WHITE : TFT_DARKGREY, TFT_BLACK);
        lcd.drawString(String(DIGITS[i]), x, y);
        if (on) lcd.drawString(String(DIGITS[i]), x + 1, y);
    }
}

void digitBarInit() {
    draw();
}

void digitBarHandleKey(char c) {
    const char *p = c ? strchr(DIGITS, c) : nullptr;
    if (p) {
        selected = p - DIGITS;
        draw();
    } else if (c == ',') {  // left arrow on keyboard (fn key)
        selected = (selected <= 0 ? DIGIT_COUNT : selected) - 1; // wraps; from "nothing" goes to last
        draw();
    } else if (c == '/') {  // right arrow on keyboard (fn key)
        selected = (selected + 1) % DIGIT_COUNT; // wraps; from "nothing" goes to first
        draw();
    }
}
