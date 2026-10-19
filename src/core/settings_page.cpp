#include "settings_page.h"

#include "page.h"
#include "settings.h"

enum Row { ROW_BACK, ROW_BRIGHTNESS, ROW_COUNT };

static int selected = ROW_BACK;

static constexpr int BAR_SEGMENTS = BRIGHTNESS_MAX;
static constexpr int BAR_SEGMENT_W = 10;
static constexpr int BAR_SEGMENT_GAP = 2;
static constexpr int BAR_W = BAR_SEGMENTS * (BAR_SEGMENT_W + BAR_SEGMENT_GAP) - BAR_SEGMENT_GAP;

static int rowY(int row) { return STATUS_BAR_H + row * PAGE_ROW_H; }

static void brightnessRowDraw() {
    auto &lcd = M5Cardputer.Display;
    int w = lcd.width();
    int y = rowY(ROW_BRIGHTNESS);

    lcd.setTextDatum(middle_left);
    lcd.setTextSize(1.5);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.drawString("brightness", 8, y + PAGE_ROW_H / 2);

    int x = w - 8 - BAR_W;
    for (int i = 0; i < BAR_SEGMENTS; i++) {
        int sx = x + i * (BAR_SEGMENT_W + BAR_SEGMENT_GAP);
        lcd.fillRect(sx, y + 5, BAR_SEGMENT_W, PAGE_ROW_H - 10, i < brightnessLevel() ? TFT_WHITE : TFT_DARKGREY);
    }
}

void settingsPageDraw() {
    pageHeaderDraw();
    brightnessRowDraw();

    // Box around the selected row
    M5Cardputer.Display.drawRect(3, rowY(selected) + 1, M5Cardputer.Display.width() - 6, PAGE_ROW_H - 2, TFT_GREEN);
}

bool settingsPageHandleKey(const Keyboard_Class::KeysState &keys) {
    // Enter goes back only on the back row; Backspace goes back from anywhere
    if ((selected == ROW_BACK || keys.del) && pageHandleBack(keys)) return true;

    bool changed = false;
    for (char c : keys.word) {
        if (c == ';') {  // up arrow (fn key)
            selected = (selected + ROW_COUNT - 1) % ROW_COUNT;
            changed = true;
        } else if (c == '.') {  // down arrow (fn key)
            selected = (selected + 1) % ROW_COUNT;
            changed = true;
        } else if (selected == ROW_BRIGHTNESS && (c == ',' || c == '/')) {  // left / right arrow (fn key)
            int before = brightnessLevel();
            brightnessSet(before + (c == '/' ? 1 : -1));
            changed |= brightnessLevel() != before;
        }
    }
    return changed;
}
