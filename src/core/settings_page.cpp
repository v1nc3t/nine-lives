#include "settings_page.h"

#include "clock.h"
#include "page.h"
#include "settings.h"

enum Row { ROW_BACK, ROW_BRIGHTNESS, ROW_SLEEP, ROW_DEEP_SLEEP, ROW_TIME, ROW_TIMEZONE, ROW_COUNT };

static int selected = ROW_BACK;

static constexpr int BAR_SEGMENTS = BRIGHTNESS_MAX;
static constexpr int BAR_SEGMENT_W = 10;
static constexpr int BAR_SEGMENT_GAP = 2;
static constexpr int BAR_W = BAR_SEGMENTS * (BAR_SEGMENT_W + BAR_SEGMENT_GAP) - BAR_SEGMENT_GAP;

static constexpr int VALUE_W = 104; // "<", the widest value ("UTC+10:30") and ">"

// Popup for setting the time: four digits (HH MM) typed on the keyboard, a cursor under one of them
static constexpr int POPUP_W = 150;
static constexpr int POPUP_H = 66;

static bool timeEditing = false;
static int timeDigits[4];
static int timeCursor;
static bool popupOnly = false; // only the popup changed since the last draw: repaint just it, so nothing flickers

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

static void valueRowDraw(Row row, const char *label, const char *value) {
    auto &lcd = M5Cardputer.Display;
    rowLabelDraw(row, label);

    // Arrows stay put; the value is centred between them
    int right = lcd.width() - 8;
    int y = rowY(row) + PAGE_ROW_H / 2;
    lcd.setTextDatum(middle_left);
    lcd.drawString("<", right - VALUE_W, y);
    lcd.setTextDatum(middle_right);
    lcd.drawString(">", right, y);
    lcd.setTextDatum(middle_center);
    lcd.drawString(value, right - VALUE_W / 2, y);
}

static void timeRowDraw() {
    rowLabelDraw(ROW_TIME, "time");
    auto &lcd = M5Cardputer.Display;
    lcd.setTextDatum(middle_right);
    lcd.drawString(clockText(), lcd.width() - 8, rowY(ROW_TIME) + PAGE_ROW_H / 2);
}

static int timeHours() { return timeDigits[0] * 10 + timeDigits[1]; }
static int timeMinutes() { return timeDigits[2] * 10 + timeDigits[3]; }
static bool timeValid() { return timeHours() < 24 && timeMinutes() < 60; }

static void timeEditOpen() {
    int now = clockLocalMinutes();
    timeDigits[0] = now / 60 / 10;
    timeDigits[1] = now / 60 % 10;
    timeDigits[2] = now % 60 / 10;
    timeDigits[3] = now % 60 % 10;
    timeCursor = 0;
    timeEditing = true;
}

static void timeDigitStep(int dir) { timeDigits[timeCursor] = (timeDigits[timeCursor] + 10 + dir) % 10; }

static void timePopupDraw() {
    auto &lcd = M5Cardputer.Display;
    int x = (lcd.width() - POPUP_W) / 2;
    int y = STATUS_BAR_H + (lcd.height() - STATUS_BAR_H - POPUP_H) / 2;
    lcd.fillRect(x, y, POPUP_W, POPUP_H, TFT_BLACK);
    lcd.drawRect(x, y, POPUP_W, POPUP_H, TFT_GREEN);

    lcd.setTextDatum(middle_center);
    lcd.setTextSize(1.5);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.drawString("set time", x + POPUP_W / 2, y + 14);

    // One fixed-width slot per character of "HH:MM", so the cursor lines up under its digit
    lcd.setTextSize(3);
    int slotW = lcd.textWidth("0");
    int left = x + (POPUP_W - 5 * slotW) / 2;
    int cy = y + 42;
    lcd.setTextColor(timeValid() ? TFT_WHITE : TFT_RED, TFT_BLACK);
    for (int i = 0; i < 5; i++) {
        int digit = i < 2 ? i : i - 1;
        char text[2] = {i == 2 ? ':' : (char)('0' + timeDigits[digit]), 0};
        lcd.drawString(text, left + i * slotW + slotW / 2, cy);
    }
    int cx = left + (timeCursor < 2 ? timeCursor : timeCursor + 1) * slotW;
    lcd.fillRect(cx, cy + 14, slotW, 2, TFT_GREEN);
}

void settingsPageDraw() {
    if (timeEditing && popupOnly) {
        popupOnly = false;
        timePopupDraw();
        return;
    }
    pageHeaderDraw();
    brightnessRowDraw();
    valueRowDraw(ROW_SLEEP, "sleep", sleepLabel());
    valueRowDraw(ROW_DEEP_SLEEP, "deep sleep", deepSleepLabel());
    timeRowDraw();
    valueRowDraw(ROW_TIMEZONE, "time zone", timezoneLabel());

    // Box around the selected row
    M5Cardputer.Display.drawRect(3, rowY(selected) + 1, M5Cardputer.Display.width() - 6, PAGE_ROW_H - 2, TFT_GREEN);

    if (timeEditing) timePopupDraw();
}

// Moves the selected row's value one step; true if it changed.
static bool valueStep(int dir) {
    int before;
    switch (selected) {
    case ROW_BRIGHTNESS:
        before = brightnessLevel();
        brightnessSet(before + dir);
        return brightnessLevel() != before;
    case ROW_SLEEP:
        before = sleepLevel();
        sleepSet(before + dir);
        return sleepLevel() != before;
    case ROW_DEEP_SLEEP:
        before = deepSleepLevel();
        deepSleepSet(before + dir);
        return deepSleepLevel() != before;
    case ROW_TIMEZONE:
        before = timezoneLevel();
        timezoneSet(before + dir);
        return timezoneLevel() != before;
    default:
        return false;
    }
}

static bool timeEditHandleKey(const Keyboard_Class::KeysState &keys) {
    if (keys.del || pageEscPressed(keys)) {
        timeEditing = false;
        return true;
    }
    if (keys.enter) {
        if (!timeValid()) return false;
        clockSet(timeHours(), timeMinutes());
        timeEditing = false;
        return true;
    }

    bool changed = false;
    for (char c : keys.word) {
        if (c >= '0' && c <= '9') {
            timeDigits[timeCursor] = c - '0';
            timeCursor = min(timeCursor + 1, 3);
            changed = true;
        } else if (c == ';' || c == '.') {  // up / down arrow (fn key): this digit +1 / -1, 9 and 0 wrap round
            timeDigitStep(c == ';' ? 1 : -1);
            changed = true;
        } else if (c == ',' && timeCursor > 0) {  // left arrow (fn key)
            timeCursor--;
            changed = true;
        } else if (c == '/' && timeCursor < 3) {  // right arrow (fn key)
            timeCursor++;
            changed = true;
        }
    }
    popupOnly = changed;
    return changed;
}

bool settingsPageHandleButton() {
    if (!timeEditing) return false;
    timeDigitStep(1);
    popupOnly = true;
    return true;
}

bool settingsPageHandleKey(const Keyboard_Class::KeysState &keys) {
    if (timeEditing) return timeEditHandleKey(keys);

    // Enter on the time row opens the popup
    if (selected == ROW_TIME && keys.enter) {
        timeEditOpen();
        return true;
    }

    // Enter goes back only on the back row; Backspace and ESC go back from anywhere
    if ((selected == ROW_BACK || !keys.enter) && pageHandleBack(keys)) return true;

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
