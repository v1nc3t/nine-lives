#include <M5Cardputer.h>

#include "core/boot.h"
#include "core/main_screen.h"
#include "core/page.h"
#include "core/page_bar.h"
#include "core/settings.h"
#include "core/status_bar.h"

// Draws the open page, or the main screen with the page bar.
static void pageDraw() {
    if (const Page *page = pageOpened()) {
        page->draw();
        return;
    }
    const Page *page = pageFor(pageBarSelectedLabel());
    if (page) {
        mainScreenDraw(page->name, page->description);
    } else {
        mainScreenDraw(nullptr, nullptr);
    }
    pageBarDraw();
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    settingsLoad();
    playBootAnimation();

    statusBarDraw();
    pageDraw();
}

void loop() {
    M5Cardputer.update();
    statusBarUpdate();

    bool keyPressed = M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed();
    bool redraw = false;

    if (const Page *page = pageOpened()) {
        if (keyPressed) redraw = page->handleKey(M5Cardputer.Keyboard.keysState());
    } else {
        bool open = M5Cardputer.BtnA.wasPressed(); // BtnGO
        if (keyPressed) {
            auto keys = M5Cardputer.Keyboard.keysState();
            for (char c : keys.word) {
                redraw |= pageBarHandleKey(c);
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
}
