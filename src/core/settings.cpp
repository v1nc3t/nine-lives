#include "settings.h"

#include <M5Cardputer.h>
#include <Preferences.h>

static constexpr const char *NAMESPACE = "nine-lives";
static constexpr int BRIGHTNESS_DEFAULT = 7;

static int brightness = BRIGHTNESS_DEFAULT;

static void brightnessApply() {
    // Squared so the low steps are really dim (step 1 is 2/255) while the top step is still full
    M5Cardputer.Display.setBrightness(brightness * brightness * 255 / (BRIGHTNESS_MAX * BRIGHTNESS_MAX));
}

void settingsLoad() {
    Preferences prefs;
    prefs.begin(NAMESPACE, true);
    brightness = constrain((int)prefs.getUChar("brightness", BRIGHTNESS_DEFAULT), BRIGHTNESS_MIN, BRIGHTNESS_MAX);
    prefs.end();
    brightnessApply();
}

int brightnessLevel() { return brightness; }

void brightnessSet(int level) {
    level = constrain(level, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
    if (level == brightness) return;
    brightness = level;
    brightnessApply();

    Preferences prefs;
    prefs.begin(NAMESPACE, false);
    prefs.putUChar("brightness", brightness);
    prefs.end();
}
