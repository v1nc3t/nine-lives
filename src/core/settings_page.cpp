#include "settings_page.h"

#include "page.h"
#include "settings.h"

enum Row { ROW_BACK, ROW_BRIGHTNESS, ROW_TIMEOUT, ROW_COUNT };

static int selected = ROW_BACK;

static constexpr int BAR_SEGMENTS = BRIGHTNESS_MAX;
static constexpr int BAR_SEGMENT_W = 10;
static constexpr int BAR_SEGMENT_GAP = 2;
static constexpr int BAR_W = BAR_SEGMENTS * (BAR_SEGMENT_W + BAR_SEGMENT_GAP) - BAR_SEGMENT_GAP;

static constexpr int TIMEOUT_VALUE_W = 70; // "<", the widest label ("never") and ">"

static int rowY(int row) { return STATUS_BAR_H + row * PAGE_ROW_H; }

static void rowLabelDraw(Row row, const char *label) {
    auto &lcd = M5Cardputer.Display;
    lcd.setTextDatum(middle_left);
    lcd.setTextSize(1.5);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.drawString(label, 8, rowY(row) + PAGE_ROW_H / 2);
}

static void brightnessRowDraw() {
    auto &lcd = M5Cardputer.Display;
    int y = rowY(ROW_BRIGHTNESS);
    rowLabelDraw(ROW_BRIGHTNESS, "brightness");

    int x = lcd.width() - 8 - BAR_W;
    for (int i = 0; i < BAR_SEGMENTS; i++) {
        int sx = x + i * (BAR_SEGMENT_W + BAR_SEGMENT_GAP);
        lcd.fillRect(sx, y + 5, BAR_SEGMENT_W, PAGE_ROW_H - 10, i < brightnessLevel() ? TFT_WHITE : TFT_DARKGREY);
    }
}

static void timeoutRowDraw() {
    auto &lcd = M5Cardputer.Display;
    rowLabelDraw(ROW_TIMEOUT, "sleep after");

    // Arrows stay put; the label is centred between them
    int right = lcd.width() - 8;
    int y = rowY(ROW_TIMEOUT) + PAGE_ROW_H / 2;
    lcd.setTextDatum(middle_left);
    lcd.drawString("<", right - TIMEOUT_VALUE_W, y);
    lcd.setTextDatum(middle_right);
    lcd.drawString(">", right, y);
    lcd.setTextDatum(middle_center);
    lcd.drawString(timeoutLabel(), right - TIMEOUT_VALUE_W / 2, y);
}

void settingsPageDraw() {
    pageHeaderDraw();
    brightnessRowDraw();
    timeoutRowDraw();

    // Box around the selected row
    M5Cardputer.Display.drawRect(3, rowY(selected) + 1, M5Cardputer.Display.width() - 6, PAGE_ROW_H - 2, TFT_GREEN);
}

// Moves the selected row's value one step; true if it changed.
static bool valueStep(int dir) {
    int before;
    switch (selected) {
    case ROW_BRIGHTNESS:
        before = brightnessLevel();
        brightnessSet(before + dir);
        return brightnessLevel() != before;
    case ROW_TIMEOUT:
        before = timeoutLevel();
        timeoutSet(before + dir);
        return timeoutLevel() != before;
    default:
        return false;
    }
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
        } else if (c == ',' || c == '/') {  // left / right arrow (fn key)
            changed |= valueStep(c == '/' ? 1 : -1);
        }
    }
    return changed;
}
