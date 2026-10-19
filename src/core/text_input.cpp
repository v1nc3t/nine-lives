#include "text_input.h"
#include "theme.h"

static constexpr int MAX_HELD = 8;

static bool caps = false;
static TextKeys fresh = {};
static Point2D_t held[MAX_HELD]; // keys down at the last change
static int heldCount = 0;
static int lastCount = 0;

bool textInputCaps() { return caps; }
const TextKeys &textInputKeys() { return fresh; }

static bool isHeld(const Point2D_t &pos) {
    for (int i = 0; i < heldCount; i++) {
        if (held[i] == pos) return true;
    }
    return false;
}

static void addChar(char c) {
    if (fresh.count < sizeof(fresh.chars)) fresh.chars[fresh.count++] = c;
}

void textInputUpdate(bool keyChanged) {
    if (!keyChanged) return;

    auto &keyboard = M5Cardputer.Keyboard;
    auto &state = keyboard.keysState();
    auto &down = keyboard.keyList();
    int count = down.size();

    fresh = {};
    for (const Point2D_t &pos : down) {
        if (isHeld(pos)) continue;
        KeyValue_t value = keyboard.getKeyValue(pos);
        uint8_t first = value.value_first;
        char c = first;
        switch (first) {
        case KEY_FN:
        case KEY_LEFT_SHIFT:
        case KEY_LEFT_CTRL:
        case KEY_LEFT_ALT:
        case KEY_TAB:
            continue;
        case KEY_OPT:
            fresh.opt = true;
            continue;
        case KEY_ENTER:
            fresh.enter = true;
            continue;
        case KEY_BACKSPACE:
            if (state.fn) fresh.forwardDel = true;
            else fresh.del = true;
            continue;
        }

        if (state.fn) { // the fn layer: arrows and ESC; nothing else types
            switch (c) {
            case ',': fresh.left = true; break;
            case '/': fresh.right = true; break;
            case ';': fresh.home = true; break;
            case '.': fresh.end = true; break;
            case '`': fresh.esc = true; break;
            }
            continue;
        }

        bool shifted = state.shift || state.ctrl;
        if (c >= 'a' && c <= 'z') shifted = shifted != caps; // caps lock turns shift round: caps + shift gives a small letter
        addChar(shifted ? value.value_second : first);
    }

    // fn + shift pressed together (just those two keys, and one of them new) toggles caps lock
    if (state.fn && state.shift && count == 2 && count > lastCount) caps = !caps;

    heldCount = min(count, MAX_HELD);
    for (int i = 0; i < heldCount; i++) held[i] = down[i];
    lastCount = count;
}

void textFieldReset(TextField &field, bool masked) {
    field = {};
    field.masked = masked;
}

bool textFieldHandleKey(TextField &field) {
    const TextKeys &keys = fresh;
    bool changed = false;

    for (int i = 0; i < keys.count; i++) {
        if (field.length >= TEXT_FIELD_MAX) break;
        memmove(field.text + field.cursor + 1, field.text + field.cursor, field.length - field.cursor + 1);
        field.text[field.cursor++] = keys.chars[i];
        field.length++;
        changed = true;
    }
    if (keys.del && field.cursor > 0) {
        memmove(field.text + field.cursor - 1, field.text + field.cursor, field.length - field.cursor + 1);
        field.cursor--;
        field.length--;
        changed = true;
    }
    if (keys.forwardDel && field.cursor < field.length) {
        memmove(field.text + field.cursor, field.text + field.cursor + 1, field.length - field.cursor);
        field.length--;
        changed = true;
    }
    if (keys.left && field.cursor > 0) {
        field.cursor--;
        changed = true;
    }
    if (keys.right && field.cursor < field.length) {
        field.cursor++;
        changed = true;
    }
    if (keys.home && field.cursor != 0) {
        field.cursor = 0;
        changed = true;
    }
    if (keys.end && field.cursor != field.length) {
        field.cursor = field.length;
        changed = true;
    }
    if (keys.opt) {
        field.masked = !field.masked;
        changed = true;
    }
    return changed;
}

void textFieldDraw(TextField &field, int x, int y, int w, int h) {
    auto &lcd = M5Cardputer.Display;
    lcd.setTextSize(1.5);
    int charW = lcd.textWidth("0");
    int visible = (w - 8) / charW;

    // Scroll just enough to keep the cursor in view
    if (field.cursor < field.first) field.first = field.cursor;
    if (field.cursor > field.first + visible - 1) field.first = field.cursor - (visible - 1);

    lcd.fillRect(x, y, w, h, TFT_BLACK);
    lcd.drawRect(x, y, w, h, TFT_WHITE);
    lcd.setTextDatum(middle_left);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);

    char shown[TEXT_FIELD_MAX + 1];
    int n = min((int)field.length - field.first, visible);
    for (int i = 0; i < n; i++) shown[i] = field.masked ? '*' : field.text[field.first + i];
    shown[n] = 0;
    lcd.drawString(shown, x + 4, y + h / 2);

    int cx = x + 4 + (field.cursor - field.first) * charW;
    lcd.fillRect(cx, y + 3, 2, h - 6, themeColor());
}
