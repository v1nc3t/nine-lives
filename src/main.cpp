#include <M5Cardputer.h>

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.drawString("Nine Lives", 10, 10);
}

void loop() {
    M5Cardputer.update();
}
