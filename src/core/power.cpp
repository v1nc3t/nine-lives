#include "power.h"

#include <M5Cardputer.h>
#include <esp_sleep.h>

#include "settings.h"

static constexpr gpio_num_t WAKE_PIN = GPIO_NUM_0; // BtnGO, pulled up, low when pressed

static uint32_t lastActivity = 0;

void powerActivity() { lastActivity = millis(); }

[[noreturn]] static void sleepNow() {
    auto &lcd = M5Cardputer.Display;
    lcd.setBrightness(0);
    lcd.sleep();
    lcd.waitDisplay();
    M5Cardputer.Speaker.end();
    M5Cardputer.Mic.end();

    esp_sleep_enable_ext0_wakeup(WAKE_PIN, 0);
    esp_deep_sleep_start();
}

void powerUpdate() {
    unsigned timeout = timeoutSeconds();
    if (timeout == 0) return;
    if (millis() - lastActivity >= timeout * 1000UL) sleepNow();
}

bool powerWokeFromSleep() { return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0; }
