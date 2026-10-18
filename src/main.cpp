#include <M5Cardputer.h>

#include "core/boot.h"
#include "core/page_bar.h"

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    playBootAnimation();
    digitBarInit();
}

void loop() {
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        for (char c : M5Cardputer.Keyboard.keysState().word) {
            digitBarHandleKey(c);
        }
    }
}
