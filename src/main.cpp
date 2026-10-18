#include <M5Cardputer.h>

#include "core/boot.h"
#include "core/main_screen.h"
#include "core/page_bar.h"
#include "core/status_bar.h"

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    playBootAnimation();

    statusBarDraw();
    mainScreenDraw(pageBarSelectedLabel());
    pageBarDraw();
}

void loop() {
    M5Cardputer.update();
    statusBarUpdate();

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        for (char c : M5Cardputer.Keyboard.keysState().word) {
            if (pageBarHandleKey(c)) {
                mainScreenDraw(pageBarSelectedLabel());
                pageBarDraw();
            }
        }
    }
}
