#include "M5Unified.hpp"
#include <M5Cardputer.h>

#include "core/boot.h"

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    playBootAnimation();
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.drawString("Nine Lives", 10, 10);
}

void loop() {
    M5Cardputer.update();
}
