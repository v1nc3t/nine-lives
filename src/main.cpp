#include <M5Cardputer.h>

#include "core/boot.h"
#include "core/clock.h"
#include "core/main_screen.h"
#include "core/page.h"
#include "core/page_bar.h"
#include "core/power.h"
#include "core/settings.h"
#include "core/status_bar.h"
#include "core/text_input.h"
#include "core/wifi.h"
#include "core/wifi_popup.h"

// Draws the open page, or the main screen with the page bar, and the Wi-Fi popup on top if it is open.
static void pageDraw() {
    if (const Page *page = pageOpened()) {
        page->draw();
    } else {
        const Page *selected = pageFor(pageBarSelectedLabel());
        if (selected) {
            mainScreenDraw(selected->name, selected->description);
        } else {
            mainScreenDraw(nullptr, nullptr);
        }
        pageBarDraw();
    }
    if (wifiPopupIsOpen()) wifiPopupDraw();
}

void setup() {
    powerBegin(); // before anything drives a pin: the ones held through deep sleep are still held
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    settingsLoad();
    clockBegin();
    if (!powerWokeFromSleep()) playBootAnimation();
    setCpuFrequencyMhz(80); // plenty for the UI, and the chip draws less (after the animation, which decodes faster at full speed)

    wifiBegin();

    statusBarDraw();
    pageDraw();
    powerActivity();
}

void loop() {
    M5Cardputer.update();
    statusBarUpdate();
    clockSyncUpdate();

    bool keyChanged = M5Cardputer.Keyboard.isChange(); // stateful: call once per loop
    textInputUpdate(keyChanged);
    if (keyChanged || M5Cardputer.BtnA.isPressed()) powerActivity();
    if (powerUpdate()) { // woke from sleep: the screen needs repainting before the backlight is back
        statusBarDraw();
        pageDraw();
    }

    bool keyPressed = keyChanged && M5Cardputer.Keyboard.isPressed();
    bool redraw = false;

    if (wifiPopupIsOpen()) { // takes over the keys, whatever it was opened from
        bool changed = keyPressed && wifiPopupHandleKey(M5Cardputer.Keyboard.keysState());
        if (M5Cardputer.BtnA.wasPressed()) changed |= wifiPopupHandleButton();
        changed |= wifiPopupUpdate();
        if (changed && wifiPopupIsOpen()) wifiPopupDraw(); // only the popup changed
        else redraw = changed;                             // it closed: the screen under it needs repainting
    } else if (const Page *page = pageOpened()) {
        if (keyPressed) redraw = page->handleKey(M5Cardputer.Keyboard.keysState());
        if (page->handleButton && M5Cardputer.BtnA.wasPressed()) redraw |= page->handleButton();
        if (page->update) redraw |= page->update();
    } else {
        bool open = M5Cardputer.BtnA.wasPressed(); // BtnGO
        if (keyPressed) {
            auto keys = M5Cardputer.Keyboard.keysState();
            for (char c : keys.word) {
                redraw |= pageBarHandleKey(c);
                if (c == '-' && !keys.fn) { // shortcut to the Wi-Fi networks
                    wifiPopupOpen();
                    redraw = true;
                }
            }
            open |= keys.enter;
        }
        if (open) {
            if (const Page *page = pageFor(pageBarSelectedLabel())) {
                pageOpen(page);
                redraw = true;
            }
        }
    }

    if (redraw) pageDraw();

    delay(10); // idle between polls instead of spinning the CPU at full speed
}
