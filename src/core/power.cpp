#include "power.h"

#include <M5Cardputer.h>
#include <esp_sleep.h>

#include "clock.h"
#include "settings.h"
#include "wifi.h"

static constexpr gpio_num_t WAKE_PIN = GPIO_NUM_0; // BtnGO, pulled up, low when pressed
static constexpr uint32_t SLEEP_POLL_MS = 50;      // how often sleep looks for a key
static constexpr uint32_t SLEEP_CLOCK_CHECK_MS = 1000;
static constexpr uint8_t SLEEP_BRIGHTNESS = 5; // of 255; dim enough to read in the dark
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
    wifiRadioOff();
    clockForget(); // the time isn't kept through deep sleep

    esp_sleep_enable_ext0_wakeup(WAKE_PIN, 0);
    esp_deep_sleep_start();
}

// Big clock on a dimmed screen and a slow CPU, checking for keys every few milliseconds. Returns on any key
// or BtnGO, or never if the deep sleep time runs out first. The screen is left black; the caller repaints it.
// With power saving on there is no clock: the screen is dark and the chip light-sleeps between key checks.
static void sleepNow() {
    auto &lcd = M5Cardputer.Display;
    uint8_t brightness = lcd.getBrightness();
    bool dark = powerSaving();
    lcd.setBrightness(dark ? 0 : SLEEP_BRIGHTNESS);
    if (!wifiEnabled()) setCpuFrequencyMhz(SLEEP_CPU_MHZ); // the radio needs the full 80 MHz to stay connected

    lcd.fillScreen(TFT_BLACK);
    lcd.setTextDatum(middle_center);
    lcd.setTextSize(7);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    char shown[6] = "";
    uint32_t lastCheck = 0;

    for (;;) {
        if (!dark && (shown[0] == 0 || millis() - lastCheck >= SLEEP_CLOCK_CHECK_MS)) {
            lastCheck = millis();
            if (strcmp(clockText(), shown) != 0) {
                strcpy(shown, clockText());
                lcd.drawString(shown, lcd.width() / 2, lcd.height() / 2);
            }
        }
        if (dark) {
            esp_sleep_enable_timer_wakeup(SLEEP_POLL_MS * 1000ULL);
            esp_light_sleep_start();
        } else {
            delay(SLEEP_POLL_MS); // not light sleep: it would stop the backlight's PWM and the clock needs the screen lit
        }
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() || M5Cardputer.BtnA.isPressed()) break;
        if (idleFor(deepSleepSeconds())) deepSleepNow();
    }

    setCpuFrequencyMhz(AWAKE_CPU_MHZ);
    lcd.fillScreen(TFT_BLACK);
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
