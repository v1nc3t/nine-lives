#include "bluetooth_popup.h"
#include "theme.h"

#include "bluetooth.h"
#include "page.h"
#include "settings.h"

static constexpr int POPUP_X = 10;
static constexpr int POPUP_Y = STATUS_BAR_H + 5;
static constexpr int POPUP_W = 220;
static constexpr int POPUP_H = 110;

static bool opened = false;
static BluetoothStatus drawn = BLUETOOTH_NONE; // status as last drawn

bool bluetoothPopupIsOpen() { return opened; }

static void close() {
    opened = false;
    bluetoothPairing(false);
}

void bluetoothPopupOpen() {
    opened = true;
    if (powerSaving()) return; // Bluetooth is off for good: only say so
    bluetoothPairing(true); // first, so advertising starts at the quick pace
    bluetoothSetEnabled(true);
}

void bluetoothPopupDraw() {
    auto &lcd = M5Cardputer.Display;
    lcd.fillRect(POPUP_X, POPUP_Y, POPUP_W, POPUP_H, TFT_BLACK);
    lcd.drawRect(POPUP_X, POPUP_Y, POPUP_W, POPUP_H, themeColor());

    int x = POPUP_X + POPUP_W / 2;
    lcd.setTextDatum(middle_center);
    lcd.setTextSize(1.5);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.drawString("bluetooth", x, POPUP_Y + 11);
    lcd.drawFastHLine(POPUP_X + 4, POPUP_Y + 21, POPUP_W - 8, TFT_DARKGREY);

    drawn = bluetoothStatus();
    const char *message = "waiting for phone";
    char pair[32];
    snprintf(pair, sizeof(pair), "pair with '%s'", BLUETOOTH_NAME);
    const char *detail = pair;
    const char *hint = "tab: off   esc: close";
    uint16_t color = TFT_WHITE;
    if (powerSaving()) {
        message = "power saving is on";
        detail = "";
    } else if (!bluetoothRunning()) {
        message = "couldn't start";
        detail = "switch it off and on again";
        color = TFT_RED;
    } else if (drawn == BLUETOOTH_PAIRED) {
        message = "not connected";
        detail = bluetoothDevice();
        hint = "enter: forget  tab: off  esc: close";
    } else if (drawn == BLUETOOTH_CONNECTED) {
        message = "connected";
        detail = bluetoothDevice();
        hint = "enter: forget  tab: off  esc: close";
        color = TFT_GREEN;
    }

    lcd.setTextColor(color, TFT_BLACK);
    lcd.drawString(message, x, POPUP_Y + 50);
    lcd.setTextSize(1);
    lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    lcd.drawString(detail, x, POPUP_Y + 68);
    lcd.drawString(hint, x, POPUP_Y + POPUP_H - 10);
}

// Enter or BtnGO
static bool action() {
    if (powerSaving()) {
        close();
        return true;
    }
    if (bluetoothStatus() != BLUETOOTH_NONE) bluetoothForget(); // redrawn by bluetoothPopupUpdate() once the phone is gone
    return false;
}

bool bluetoothPopupHandleKey(const Keyboard_Class::KeysState &keys) {
    if (keys.tab && !powerSaving()) { // power saving: Bluetooth is off already
        bluetoothSetEnabled(false);
        close();
        return true;
    }
    if (keys.del || pageEscPressed(keys)) {
        close();
        return true;
    }
    return keys.enter && action();
}

bool bluetoothPopupHandleButton() { return action(); }

bool bluetoothPopupUpdate() { return bluetoothStatus() != drawn; }
