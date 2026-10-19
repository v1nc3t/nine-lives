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

static int brightness = BRIGHTNESS_DEFAULT;
static int sleepStep = SLEEP_DEFAULT;
static int deepSleepStep = DEEP_SLEEP_DEFAULT;
static int timezone = TIMEZONE_UTC;
static bool wifi = false;

static void brightnessApply() {
    // Squared so the low steps are really dim (step 1 is 2/255) while the top step is still full
    M5Cardputer.Display.setBrightness(brightness * brightness * 255 / (BRIGHTNESS_MAX * BRIGHTNESS_MAX));
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
    wifi = prefs.getUChar("wifi", 0) != 0;
    prefs.end();
    brightnessApply();
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

bool wifiOn() { return wifi; }

void wifiOnSet(bool on) {
    if (on == wifi) return;
    wifi = on;
    save("wifi", wifi);
}
