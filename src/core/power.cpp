#include "power.h"

#include <M5Cardputer.h>
#include <driver/gpio.h>
#include <esp_sleep.h>

#include "clock.h"
#include "settings.h"
#include "sound.h"
#include "wifi.h"

static constexpr gpio_num_t WAKE_PIN = GPIO_NUM_0; // BtnGO, pulled up, low when pressed
static constexpr gpio_num_t IR_PIN = GPIO_NUM_44;  // IR LED; unused, so kept low (off) instead of left as the serial port's input

// Every pin the firmware drives, with the idle level to park it at for deep sleep. The chip lets go of all of its pins
// when it powers down, and a floating one can leave the backlight, speaker amp, keyboard decoder or screen drawing milliamps.
struct ParkedPin {
    gpio_num_t pin;
    int level;
};
static constexpr ParkedPin PARKED_PINS[] = {
    {GPIO_NUM_38, 0}, // backlight
    {GPIO_NUM_21, 0}, // RGB LED data
    {GPIO_NUM_8, 0},  // keyboard decoder address
    {GPIO_NUM_9, 0},
    {GPIO_NUM_11, 0},
    {GPIO_NUM_41, 0}, // speaker amp: bit clock
    {GPIO_NUM_42, 0}, // ... data
    {GPIO_NUM_43, 0}, // ... word clock, also the microphone's clock
    {GPIO_NUM_33, 1}, // screen reset (high: not in reset)
    {GPIO_NUM_34, 0}, // ... data/command
    {GPIO_NUM_35, 0}, // ... data
    {GPIO_NUM_36, 0}, // ... clock
    {GPIO_NUM_37, 1}, // ... chip select (high: not selected)
    {GPIO_NUM_12, 1}, // SD card chip select (high: not selected)
    {GPIO_NUM_40, 0}, // ... clock
    {GPIO_NUM_14, 0}, // ... data in
    {IR_PIN, 0},      // IR LED
};
static constexpr uint32_t SLEEP_POLL_MS = 50;       // how often sleep looks for a key
static constexpr uint32_t DARK_SLEEP_POLL_MS = 100; // ... and with power saving on, where each look wakes the chip from light sleep (BtnGO wakes it at once)
static constexpr uint32_t SLEEP_CLOCK_CHECK_MS = 1000;
static constexpr uint8_t SLEEP_BRIGHTNESS = 5; // of 255; dim enough to read in the dark
static constexpr uint32_t SLEEP_CPU_MHZ = 20;
static constexpr uint32_t AWAKE_CPU_MHZ = 80;
static constexpr uint32_t HOLD_SLEEP_MS = 5000; // holding BtnGO this long deep-sleeps at once

static uint32_t lastActivity = 0;
static bool goReleased = false; // a hold only counts once BtnGO has been let go, so the press that woke the chip doesn't send it back to sleep

void powerActivity() { lastActivity = millis(); }

static bool idleFor(unsigned seconds) { return seconds != 0 && millis() - lastActivity >= seconds * 1000UL; }

// Drives each pin to its idle level and holds it there through deep sleep.
static void parkPins() {
    for (const auto &parked : PARKED_PINS) {
        gpio_set_direction(parked.pin, GPIO_MODE_OUTPUT);
        gpio_set_level(parked.pin, parked.level);
        gpio_hold_en(parked.pin);
    }
    gpio_deep_sleep_hold_en();
}

void powerBegin() {
    gpio_deep_sleep_hold_dis();
    for (const auto &parked : PARKED_PINS) gpio_hold_dis(parked.pin);
    pinMode(IR_PIN, OUTPUT);
    digitalWrite(IR_PIN, LOW);
}

[[noreturn]] static void deepSleepNow() {
    auto &lcd = M5Cardputer.Display;
    lcd.setBrightness(0);
    ledOff();
    lcd.sleep();
    lcd.waitDisplay();
    soundPowerDown();
    M5Cardputer.Mic.end();
    wifiRadioOff();
    clockForget(); // the time isn't kept through deep sleep
    parkPins();

    while (digitalRead(WAKE_PIN) == LOW) delay(10); // a hold of BtnGO brought us here: its low level would wake the chip at once
    delay(50);                                      // ... and so would its contact bounce

    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL); // the light sleep's timer would wake (and reboot) the chip at once
    esp_sleep_enable_ext0_wakeup(WAKE_PIN, 0);
    esp_deep_sleep_start();
}

// Big clock on a dimmed screen and a slow CPU, checking for keys every few milliseconds. Returns on any key
// or BtnGO, or never if the deep sleep time runs out first. The screen is left black; the caller repaints it.
// With power saving on there is no clock: the screen's controller sleeps too and the chip light-sleeps between key checks.
// Either way the speaker and the LED are off, and a connected Wi-Fi radio wakes for fewer of the router's beacons.
static void sleepNow() {
    auto &lcd = M5Cardputer.Display;
    uint8_t brightness = lcd.getBrightness();
    bool dark = powerSaving();
    soundPowerDown();
    ledOff();
    wifiLowPower(true);
    if (!wifiEnabled()) setCpuFrequencyMhz(SLEEP_CPU_MHZ); // the radio needs the full 80 MHz to stay connected

    lcd.setBrightness(0);
    lcd.fillScreen(TFT_BLACK);
    if (dark) {
        lcd.sleep(); // the controller stops refreshing the panel; what it shows is kept
        esp_sleep_enable_timer_wakeup(DARK_SLEEP_POLL_MS * 1000ULL);
        gpio_wakeup_enable(WAKE_PIN, GPIO_INTR_LOW_LEVEL);
        esp_sleep_enable_gpio_wakeup();
    } else {
        lcd.powerSaveOn(); // idle mode: 8 colors, plenty for a white clock on black, and the controller draws less
        lcd.setBrightness(SLEEP_BRIGHTNESS);
    }

    lcd.setTextDatum(middle_center);
    lcd.setTextSize(7);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    char shown[6] = "";
    uint32_t lastCheck = 0;

    for (;;) {
        if (dark) {
            esp_light_sleep_start();
        } else {
            if (shown[0] == 0 || millis() - lastCheck >= SLEEP_CLOCK_CHECK_MS) {
                lastCheck = millis();
                if (strcmp(clockText(), shown) != 0) {
                    strcpy(shown, clockText());
                    lcd.drawString(shown, lcd.width() / 2, lcd.height() / 2);
                }
            }
            delay(SLEEP_POLL_MS); // not light sleep: it would stop the backlight's PWM and the clock needs the screen lit
        }
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() || M5Cardputer.BtnA.isPressed()) break;
        if (idleFor(deepSleepSeconds())) deepSleepNow();
    }

    if (dark) {
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
        gpio_wakeup_disable(WAKE_PIN);
        lcd.wakeup();
        delay(5); // the controller takes this long to come out of sleep before it takes commands
    } else {
        lcd.powerSaveOff();
    }
    setCpuFrequencyMhz(AWAKE_CPU_MHZ);
    wifiLowPower(false);
    lcd.fillScreen(TFT_BLACK);
    lcd.setBrightness(brightness);
    ledOn();
    powerActivity();
}

bool powerUpdate() {
    auto &go = M5Cardputer.BtnA;
    if (!go.isPressed()) goReleased = true;
    else if (goReleased && go.pressedFor(HOLD_SLEEP_MS)) deepSleepNow();
    if (idleFor(deepSleepSeconds())) deepSleepNow();
    if (!idleFor(sleepSeconds())) return false;
    sleepNow();
    return true;
}

bool powerWokeFromSleep() { return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0; }
