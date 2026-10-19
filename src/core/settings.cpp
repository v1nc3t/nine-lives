#include "settings.h"

#include <M5Cardputer.h>
#include <Preferences.h>

static constexpr const char *NAMESPACE = "nine-lives";
static constexpr int BRIGHTNESS_DEFAULT = 7;
static constexpr int SLEEP_DEFAULT = 1;      // 30s
static constexpr int DEEP_SLEEP_DEFAULT = 4; // 5m
static constexpr int TIMEZONE_STEPS = 53;    // UTC-12:00 .. UTC+14:00 every 30 minutes
static constexpr int TIMEZONE_UTC = 24;      // the step for UTC itself, and the default

struct Timeout {
    const char *label;
    uint16_t seconds;
};
static const Timeout TIMEOUTS[] = {{"15s", 15}, {"30s", 30}, {"1m", 60}, {"2m", 120}, {"5m", 300}, {"10m", 600}, {"30m", 1800}, {"never", 0}};
static constexpr int TIMEOUT_COUNT = sizeof(TIMEOUTS) / sizeof(TIMEOUTS[0]);

struct LedColor {
    const char *label;
    uint32_t rgb;
};
static const LedColor LED_COLORS[] = {{"red", 0xFF0000},    {"orange", 0xFF6000}, {"yellow", 0xFFD000}, {"green", 0x00FF00}, {"cyan", 0x00FFFF},
                                      {"blue", 0x0000FF},   {"purple", 0x8000FF}, {"pink", 0xFF40A0},   {"white", 0xFFFFFF}};
static constexpr int LED_COLOR_COUNT = sizeof(LED_COLORS) / sizeof(LED_COLORS[0]);
static constexpr int LED_DEFAULT = 0;       // off
static constexpr int LED_COLOR_DEFAULT = 4; // cyan

static int brightness = BRIGHTNESS_DEFAULT;
static int sleepStep = SLEEP_DEFAULT;
static int deepSleepStep = DEEP_SLEEP_DEFAULT;
static int timezone = TIMEZONE_UTC;
static int led = LED_DEFAULT;
static int ledColor = LED_COLOR_DEFAULT;
static bool wifi = false;
static bool saving = false;

static void brightnessApply() {
    // Squared so the low steps are really dim (step 1 is 2/255) while the top step is still full
    M5Cardputer.Display.setBrightness(brightness * brightness * 255 / (BRIGHTNESS_MAX * BRIGHTNESS_MAX));
}

// The RGB LED is the Stamp's, on this pin. M5Unified's own LED driver does nothing with this Arduino core (it needs ESP-IDF 5).
static constexpr uint8_t LED_PIN = 21;

static void ledApply() {
    // Squared like the screen, so the low steps are really dim (the LED is blinding at full)
    uint32_t scale = led * led; // 0 .. LED_MAX^2
    uint32_t rgb = LED_COLORS[ledColor].rgb;
    auto channel = [&](int shift) { return (uint8_t)(((rgb >> shift) & 0xFF) * scale / (LED_MAX * LED_MAX)); };
    neopixelWrite(LED_PIN, channel(16), channel(8), channel(0));
}

static void save(const char *key, int value) {
    Preferences prefs;
    prefs.begin(NAMESPACE, false);
    prefs.putUChar(key, value);
    prefs.end();
}

void settingsLoad() {
    Preferences prefs;
    prefs.begin(NAMESPACE, true);
    brightness = constrain((int)prefs.getUChar("brightness", BRIGHTNESS_DEFAULT), BRIGHTNESS_MIN, BRIGHTNESS_MAX);
    sleepStep = constrain((int)prefs.getUChar("sleep", SLEEP_DEFAULT), 0, TIMEOUT_COUNT - 1);
    deepSleepStep = constrain((int)prefs.getUChar("deepsleep", DEEP_SLEEP_DEFAULT), sleepStep, TIMEOUT_COUNT - 1);
    timezone = constrain((int)prefs.getUChar("timezone", TIMEZONE_UTC), 0, TIMEZONE_STEPS - 1);
    led = constrain((int)prefs.getUChar("led", LED_DEFAULT), LED_MIN, LED_MAX);
    ledColor = constrain((int)prefs.getUChar("ledcolor", LED_COLOR_DEFAULT), 0, LED_COLOR_COUNT - 1);
    wifi = prefs.getUChar("wifi", 0) != 0;
    saving = prefs.getUChar("saving", 0) != 0;
    prefs.end();
    brightnessApply();
    ledApply();
}

int brightnessLevel() { return brightness; }

void brightnessSet(int level) {
    level = constrain(level, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
    if (level == brightness) return;
    brightness = level;
    brightnessApply();
    save("brightness", brightness);
}

int timeoutCount() { return TIMEOUT_COUNT; }

static void timeoutSet(int &step, const char *key, int level) {
    level = constrain(level, 0, TIMEOUT_COUNT - 1);
    if (level == step) return;
    step = level;
    save(key, step);
}

void deepSleepSet(int level);

int sleepLevel() { return sleepStep; }
void sleepSet(int level) {
    timeoutSet(sleepStep, "sleep", level);
    deepSleepSet(deepSleepStep); // pushes deep sleep up if sleep now passed it
}
const char *sleepLabel() { return TIMEOUTS[sleepStep].label; }
unsigned sleepSeconds() { return TIMEOUTS[sleepStep].seconds; }

int deepSleepLevel() { return deepSleepStep; }
void deepSleepSet(int level) { timeoutSet(deepSleepStep, "deepsleep", max(level, sleepStep)); }
const char *deepSleepLabel() { return TIMEOUTS[deepSleepStep].label; }
unsigned deepSleepSeconds() { return TIMEOUTS[deepSleepStep].seconds; }

int timezoneLevel() { return timezone; }
int timezoneMinutes() { return (timezone - TIMEZONE_UTC) * 30; }

void timezoneSet(int level) {
    level = constrain(level, 0, TIMEZONE_STEPS - 1);
    if (level == timezone) return;
    timezone = level;
    save("timezone", timezone);
}

const char *timezoneLabel() {
    static char label[12];
    int minutes = timezoneMinutes();
    int abs = minutes < 0 ? -minutes : minutes;
    if (minutes == 0) snprintf(label, sizeof(label), "UTC");
    else if (abs % 60 == 0) snprintf(label, sizeof(label), "UTC%c%d", minutes < 0 ? '-' : '+', abs / 60);
    else snprintf(label, sizeof(label), "UTC%c%d:%02d", minutes < 0 ? '-' : '+', abs / 60, abs % 60);
    return label;
}

bool powerSaving() { return saving; }

void powerSavingSet(bool on) {
    if (on == saving) return;
    saving = on;
    save("saving", saving);
}

bool wifiOn() { return wifi; }

void wifiOnSet(bool on) {
    if (on == wifi) return;
    wifi = on;
    save("wifi", wifi);
}

int ledLevel() { return led; }

void ledSet(int level) {
    level = constrain(level, LED_MIN, LED_MAX);
    if (level == led) return;
    led = level;
    ledApply();
    save("led", led);
}

int ledColorCount() { return LED_COLOR_COUNT; }
int ledColorLevel() { return ledColor; }

void ledColorSet(int level) {
    level = (level % LED_COLOR_COUNT + LED_COLOR_COUNT) % LED_COLOR_COUNT; // wraps round: after white comes red
    if (level == ledColor) return;
    ledColor = level;
    ledApply();
    save("ledcolor", ledColor);
}

const char *ledColorLabel() { return LED_COLORS[ledColor].label; }
uint32_t ledColorRgb() { return LED_COLORS[ledColor].rgb; }

void ledOff() { neopixelWrite(LED_PIN, 0, 0, 0); }
