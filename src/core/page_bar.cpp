#include "page_bar.h"

#include <M5Cardputer.h>

static const char DIGITS[] = "1234567890";
static constexpr int DIGIT_COUNT = sizeof(DIGITS) - 1;

static int selected = -1; // index into DIGITS, -1 = nothing pressed yet

void pageBarDraw() {
    auto &lcd = M5Cardputer.Display;
    int w = lcd.width();
    int top = lcd.height() - PAGE_BAR_H;

    lcd.fillRect(0, top, w, PAGE_BAR_H, TFT_BLACK);
    lcd.setTextDatum(middle_center);

    // The GFX fonts have no bold, so the selected digit is drawn twice, 1 px apart
    lcd.setTextSize(1.2);
    int cell = w / DIGIT_COUNT;
    int y = top + PAGE_BAR_H / 2;
    for (int i = 0; i < DIGIT_COUNT; i++) {
        int x = i * cell + cell / 2;
        bool on = i == selected;
        lcd.setTextColor(on ? TFT_WHITE : TFT_DARKGREY, TFT_BLACK);
        lcd.drawString(String(DIGITS[i]), x, y);
        if (on) lcd.drawString(String(DIGITS[i]), x + 1, y);
    }
}

bool pageBarHandleKey(char c) {
    const char *p = c ? strchr(DIGITS, c) : nullptr;
    if (p) {
        selected = p - DIGITS;
    } else if (c == ',') {  // left arrow on keyboard (fn key)
        selected = (selected <= 0 ? DIGIT_COUNT : selected) - 1; // wraps; from "nothing" goes to last
    } else if (c == '/') {  // right arrow on keyboard (fn key)
        selected = (selected + 1) % DIGIT_COUNT; // wraps; from "nothing" goes to first
    } else {
        return false;
    }
    return true;
}

char pageBarSelectedLabel() {
    return selected >= 0 ? DIGITS[selected] : 0;
}
