#include "power.h"

#include <M5Cardputer.h>
#include <esp_sleep.h>

#include "settings.h"

static constexpr gpio_num_t WAKE_PIN = GPIO_NUM_0; // BtnGO, pulled up, low when pressed
static constexpr uint32_t SLEEP_POLL_MS = 50;      // how often sleep wakes briefly to look for a key
static constexpr uint32_t SLEEP_CPU_MHZ = 20;
static constexpr uint32_t AWAKE_CPU_MHZ = 80;

static uint32_t lastActivity = 0;

void powerActivity() { lastActivity = millis(); }

static bool idleFor(unsigned seconds) { return seconds != 0 && millis() - lastActivity >= seconds * 1000UL; }

[[noreturn]] static void deepSleepNow() {
    auto &lcd = M5Cardputer.Display;
    lcd.setBrightness(0);
    lcd.sleep();
    lcd.waitDisplay();
    M5Cardputer.Speaker.end();
    M5Cardputer.Mic.end();

    esp_sleep_enable_ext0_wakeup(WAKE_PIN, 0);
    esp_deep_sleep_start();
}

// Screen off, slow CPU, and the chip in light sleep between short key checks. Returns on any key or BtnGO,
// or never if the deep sleep time runs out first.
static void sleepNow() {
    auto &lcd = M5Cardputer.Display;
    uint8_t brightness = lcd.getBrightness();
    lcd.setBrightness(0);
    lcd.sleep();
    lcd.waitDisplay();
    setCpuFrequencyMhz(SLEEP_CPU_MHZ);

    esp_sleep_enable_timer_wakeup(SLEEP_POLL_MS * 1000ULL);
    for (;;) {
        esp_light_sleep_start();
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() || M5Cardputer.BtnA.isPressed()) break;
        if (idleFor(deepSleepSeconds())) deepSleepNow();
    }

    setCpuFrequencyMhz(AWAKE_CPU_MHZ);
    lcd.wakeup();
    lcd.setBrightness(brightness);
    powerActivity();
}

bool powerUpdate() {
    if (idleFor(deepSleepSeconds())) deepSleepNow();
    if (!idleFor(sleepSeconds())) return false;
    sleepNow();
    return true;
}

bool powerWokeFromSleep() { return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0; }
