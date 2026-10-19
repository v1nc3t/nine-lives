#include "settings.h"

#include <M5Cardputer.h>
#include <Preferences.h>

static constexpr const char *NAMESPACE = "nine-lives";
static constexpr int BRIGHTNESS_DEFAULT = 7;
static constexpr int TIMEOUT_DEFAULT = 1; // 30s

struct Timeout {
    const char *label;
    uint16_t seconds;
};
static const Timeout TIMEOUTS[] = {{"15s", 15}, {"30s", 30}, {"1m", 60}, {"2m", 120}, {"5m", 300}, {"never", 0}};
static constexpr int TIMEOUT_COUNT = sizeof(TIMEOUTS) / sizeof(TIMEOUTS[0]);

static int brightness = BRIGHTNESS_DEFAULT;
static int timeout = TIMEOUT_DEFAULT;

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
    timeout = constrain((int)prefs.getUChar("timeout", TIMEOUT_DEFAULT), 0, TIMEOUT_COUNT - 1);
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
int timeoutLevel() { return timeout; }
const char *timeoutLabel() { return TIMEOUTS[timeout].label; }
unsigned timeoutSeconds() { return TIMEOUTS[timeout].seconds; }

void timeoutSet(int level) {
    level = constrain(level, 0, TIMEOUT_COUNT - 1);
    if (level == timeout) return;
    timeout = level;
    save("timeout", timeout);
}
