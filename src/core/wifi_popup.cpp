#include "wifi_popup.h"

#include "page.h"
#include "text_input.h"
#include "wifi.h"

enum State { CLOSED, SCANNING, LIST, PASSWORD, CONNECTING, FAILED };

static constexpr int POPUP_X = 10;
static constexpr int POPUP_Y = STATUS_BAR_H + 5;
static constexpr int POPUP_W = 220;
static constexpr int POPUP_H = 110;
static constexpr int TITLE_MAX_CHARS = 20;

static constexpr int LIST_Y = POPUP_Y + 24;
static constexpr int LIST_ROW_H = 16;
static constexpr int LIST_ROWS = 5;

static State state = CLOSED;
static int selected = 0; // in the list
static int top = 0;      // first list row shown
static char ssid[33];    // the network chosen from the list
static bool secured;
static TextField password;
static bool shownCaps; // caps lock as last drawn

bool wifiPopupIsOpen() { return state != CLOSED; }

static void close() {
    state = CLOSED;
    textFieldReset(password, true);
    wifiRejoin(); // a scan or a cancelled join may have dropped the connection
}

void wifiPopupOpen() {
    wifiScanStart();
    selected = 0;
    top = 0;
    textFieldReset(password, true);
    state = SCANNING;
}

// Cuts text to max characters, ending in ".." when it was longer
static void fit(char *out, const char *text, int max) {
    if ((int)strlen(text) <= max) {
        strcpy(out, text);
    } else {
        memcpy(out, text, max - 2);
        strcpy(out + max - 2, "..");
    }
}

static void titleDraw(const char *text) {
    auto &lcd = M5Cardputer.Display;
    char title[TITLE_MAX_CHARS + 1];
    fit(title, text, TITLE_MAX_CHARS);
    lcd.setTextDatum(middle_center);
    lcd.setTextSize(1.5);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.drawString(title, POPUP_X + POPUP_W / 2, POPUP_Y + 11);
    lcd.drawFastHLine(POPUP_X + 4, POPUP_Y + 21, POPUP_W - 8, TFT_DARKGREY);
}

static void messageDraw(const char *message, uint16_t color, const char *hint) {
    auto &lcd = M5Cardputer.Display;
    lcd.setTextDatum(middle_center);
    lcd.setTextSize(1.5);
    lcd.setTextColor(color, TFT_BLACK);
    lcd.drawString(message, POPUP_X + POPUP_W / 2, POPUP_Y + 55);
    lcd.setTextSize(1);
    lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    lcd.drawString(hint, POPUP_X + POPUP_W / 2, POPUP_Y + POPUP_H - 10);
}

static void padlockDraw(int x, int y, uint16_t color) {
    auto &lcd = M5Cardputer.Display;
    lcd.drawRect(x + 1, y, 5, 5, color);
    lcd.fillRect(x, y + 3, 7, 5, color);
}

// Marks a network whose password is remembered
static void checkDraw(int x, int y, uint16_t color) {
    auto &lcd = M5Cardputer.Display;
    lcd.drawLine(x, y + 4, x + 2, y + 6, color);
    lcd.drawLine(x + 2, y + 6, x + 6, y, color);
    lcd.drawLine(x, y + 5, x + 2, y + 7, color);
    lcd.drawLine(x + 2, y + 7, x + 6, y + 1, color);
}

static void signalDraw(int right, int bottom, int rssi, uint16_t color) {
    int level = rssi >= -55 ? 4 : rssi >= -65 ? 3 : rssi >= -75 ? 2 : 1;
    for (int i = 0; i < 4; i++) {
        int h = 2 + i * 2;
        M5Cardputer.Display.fillRect(right - 15 + i * 4, bottom - h, 3, h, i < level ? color : TFT_DARKGREY);
    }
}

static void listDraw() {
    auto &lcd = M5Cardputer.Display;
    int count = wifiScanPoll();
    if (count == 0) {
        messageDraw("no networks found", TFT_WHITE, "enter: scan again   esc: close");
        return;
    }

    lcd.setTextDatum(middle_left);
    lcd.setTextSize(1.5);
    int charW = lcd.textWidth("0");
    int nameX = POPUP_X + 20;
    int signalRight = POPUP_X + POPUP_W - 8;
    int nameMax = (signalRight - 20 - nameX) / charW;

    for (int row = 0; row < LIST_ROWS && top + row < count; row++) {
        const WifiNetwork &net = wifiNetwork(top + row);
        int y = LIST_Y + row * LIST_ROW_H;
        bool on = top + row == selected;
        uint16_t color = on ? TFT_GREEN : TFT_WHITE;

        if (on) lcd.drawRect(POPUP_X + 3, y, POPUP_W - 6, LIST_ROW_H, TFT_GREEN);
        if (wifiPasswordSaved(net.ssid)) checkDraw(POPUP_X + 8, y + 4, color);
        else if (net.secured) padlockDraw(POPUP_X + 8, y + 4, color);
        char name[33];
        fit(name, net.ssid, nameMax);
        lcd.setTextColor(color, TFT_BLACK);
        lcd.drawString(name, nameX, y + LIST_ROW_H / 2);
        signalDraw(signalRight, y + LIST_ROW_H - 4, net.rssi, color);
    }
}

static void passwordDraw() {
    auto &lcd = M5Cardputer.Display;
    lcd.setTextDatum(middle_left);
    lcd.setTextSize(1);
    lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    lcd.drawString("password", POPUP_X + 8, POPUP_Y + 33);
    shownCaps = textInputCaps();
    if (shownCaps) {
        lcd.setTextDatum(middle_right);
        lcd.setTextColor(TFT_GREEN, TFT_BLACK);
        lcd.drawString("CAPS", POPUP_X + POPUP_W - 8, POPUP_Y + 33);
    }

    textFieldDraw(password, POPUP_X + 8, POPUP_Y + 42, POPUP_W - 16, 24);

    lcd.setTextDatum(middle_center);
    lcd.setTextSize(1);
    lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    lcd.drawString("opt: show   fn+`: esc", POPUP_X + POPUP_W / 2, POPUP_Y + 80);
}

void wifiPopupDraw() {
    auto &lcd = M5Cardputer.Display;
    lcd.fillRect(POPUP_X, POPUP_Y, POPUP_W, POPUP_H, TFT_BLACK);
    lcd.drawRect(POPUP_X, POPUP_Y, POPUP_W, POPUP_H, TFT_GREEN);

    switch (state) {
    case SCANNING:
        titleDraw("wifi");
        messageDraw("scanning...", TFT_WHITE, "esc: cancel");
        break;
    case LIST:
        titleDraw("networks");
        listDraw();
        break;
    case PASSWORD:
        titleDraw(ssid);
        passwordDraw();
        break;
    case CONNECTING:
        titleDraw(ssid);
        messageDraw("connecting...", TFT_WHITE, "esc: cancel");
        break;
    case FAILED:
        titleDraw(ssid);
        messageDraw("couldn't connect", TFT_RED, "any key: try again");
        break;
    case CLOSED:
        break;
    }
}

static void join() {
    wifiConnect(ssid, password.text);
    state = CONNECTING;
}

static void select() {
    if (wifiScanPoll() == 0) { // nothing to pick: look again
        wifiPopupOpen();
        return;
    }
    const WifiNetwork &net = wifiNetwork(selected);
    strcpy(ssid, net.ssid);
    secured = net.secured;
    textFieldReset(password, true);
    if (!secured) join();
    else if (wifiPasswordSaved(ssid)) {
        wifiConnectSaved(ssid);
        state = CONNECTING;
    } else state = PASSWORD;
}

static bool listHandleKey(const Keyboard_Class::KeysState &keys) {
    if (keys.del || pageEscPressed(keys)) {
        close();
        return true;
    }
    if (keys.enter) {
        select();
        return true;
    }

    int count = wifiScanPoll();
    if (count == 0) return false;
    bool changed = false;
    for (char c : keys.word) {
        if (c == ';') {  // up arrow (fn key)
            selected = (selected + count - 1) % count;
            changed = true;
        } else if (c == '.') {  // down arrow (fn key)
            selected = (selected + 1) % count;
            changed = true;
        }
    }
    if (selected < top) top = selected;
    if (selected >= top + LIST_ROWS) top = selected - LIST_ROWS + 1;
    return changed;
}

static bool passwordHandleKey() {
    const TextKeys &keys = textInputKeys();
    if (keys.esc) {
        state = LIST;
        return true;
    }
    if (keys.enter) {
        if (password.length == 0) return false;
        join();
        return true;
    }
    bool changed = textFieldHandleKey(password);
    return changed || textInputCaps() != shownCaps; // caps lock toggled
}

bool wifiPopupHandleKey(const Keyboard_Class::KeysState &keys) {
    switch (state) {
    case SCANNING:
        if (!keys.del && !pageEscPressed(keys)) return false;
        close();
        return true;
    case LIST:
        return listHandleKey(keys);
    case PASSWORD:
        return passwordHandleKey();
    case CONNECTING:
        if (!keys.del && !pageEscPressed(keys)) return false;
        wifiConnectCancel();
        state = secured ? PASSWORD : LIST;
        return true;
    case FAILED:
        state = secured ? PASSWORD : LIST;
        return true;
    case CLOSED:
        break;
    }
    return false;
}

bool wifiPopupHandleButton() {
    switch (state) {
    case LIST:
        select();
        return true;
    case PASSWORD:
        if (password.length == 0) return false;
        join();
        return true;
    default:
        return false;
    }
}

bool wifiPopupUpdate() {
    if (state == SCANNING && wifiScanPoll() >= 0) {
        state = LIST;
        return true;
    }
    if (state == CONNECTING) {
        int result = wifiConnectPoll();
        if (result > 0) close();
        else if (result < 0) state = FAILED;
        return result != 0;
    }
    return false;
}
