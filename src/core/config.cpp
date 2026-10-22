#include "config.h"

#include <SD.h>
#include <cstdlib>
#include <cstring>

#include "sd.h"
#include "settings.h"

static constexpr const char *PATH = "/nine-lives.conf";
static constexpr const char *TEMP = "/nine-lives.tmp"; // written whole first, so a pulled card never leaves a half file

struct Setting {
    const char *key;
    int (*get)();
    void (*set)(int); // clamps the value and applies it
};

// Loaded in this order: sleep before deep sleep (which is never earlier), and the theme before the LED color (a theme switches the LED color)
static const Setting SETTINGS[] = {
    {"brightness", brightnessLevel, brightnessSet},
    {"sleep", sleepLevel, sleepSet},
    {"deepsleep", deepSleepLevel, deepSleepSet},
    {"timezone", timezoneLevel, timezoneSet},
    {"theme", themeLevel, themeSet},
    {"led", ledLevel, ledSet},
    {"ledcolor", ledColorLevel, ledColorSet},
    {"sound", [] { return (int)soundOn(); }, [](int on) { soundOnSet(on != 0); }},
    {"soundchoice", soundLevel, soundSet},
    {"volume", volumeLevel, volumeSet},
    {"wifi", [] { return (int)wifiOn(); }, [](int on) { wifiOnSet(on != 0); }},
    {"bluetooth", [] { return (int)bluetoothOn(); }, [](int on) { bluetoothOnSet(on != 0); }},
    {"saving", [] { return (int)powerSaving(); }, [](int on) { powerSavingSet(on != 0); }},
};
static constexpr int SETTING_COUNT = sizeof(SETTINGS) / sizeof(SETTINGS[0]);

static bool loading = false; // the setters save to flash, and so would write the file once per setting

void configSave() {
    if (loading || !sdMounted()) return;

    // shortcut: 512 bytes hold about 25 settings, a longer table is cut short; raise it when the table grows
    char text[512];
    size_t length = snprintf(text, sizeof(text), "# Nine Lives settings. The app rewrites this file on every change, so comments do not survive.\n");
    for (const Setting &setting : SETTINGS) {
        length += snprintf(text + length, sizeof(text) - length, "%s=%d\n", setting.key, setting.get());
        if (length >= sizeof(text)) {
            length = sizeof(text) - 1;
            break;
        }
    }

    File file = SD.open(TEMP, FILE_WRITE);
    if (!file) { // the card was pulled or is broken: sdMount() looks again
        sdUnmount();
        return;
    }
    size_t written = file.write((const uint8_t *)text, length);
    file.close();
    if (written != length) {
        SD.remove(TEMP);
        sdUnmount();
        return;
    }
    SD.remove(PATH);
    SD.rename(TEMP, PATH);
}

void configLoad() {
    if (!sdMounted()) return;
    File file = SD.open(PATH);
    if (!file) {
        configSave();
        return;
    }

    // Read everything first, then apply in the table's order whatever the order in the file is
    int value[SETTING_COUNT];
    bool found[SETTING_COUNT] = {};
    char line[64];
    while (file.available()) {
        int length = file.readBytesUntil('\n', line, sizeof(line) - 1);
        line[length] = 0;
        char *equals = strchr(line, '=');
        if (line[0] == '#' || !equals) continue;
        char *keyEnd = equals;
        while (keyEnd > line && keyEnd[-1] == ' ') keyEnd--;
        *keyEnd = 0;
        char *end;
        long number = strtol(equals + 1, &end, 10);
        if (end == equals + 1) continue; // not a number: leave that setting as it is
        for (int i = 0; i < SETTING_COUNT; i++) {
            if (strcmp(line, SETTINGS[i].key) == 0) {
                value[i] = number;
                found[i] = true;
            }
        }
    }
    file.close();

    loading = true;
    for (int i = 0; i < SETTING_COUNT; i++) {
        if (found[i]) SETTINGS[i].set(value[i]);
    }
    loading = false;
}
