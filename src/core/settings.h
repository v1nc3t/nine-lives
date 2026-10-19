#pragma once

#include <cstdint>

// User settings, kept in the chip's flash (NVS) so they survive a reboot.

// Call once in setup() after M5Cardputer.begin(): applies the saved settings.
void settingsLoad();

// Screen brightness as a step from BRIGHTNESS_MIN to BRIGHTNESS_MAX (never fully off, so the screen can't be lost).
static constexpr int BRIGHTNESS_MIN = 1;
static constexpr int BRIGHTNESS_MAX = 10;
int brightnessLevel();
void brightnessSet(int level); // clamps, applies to the screen and saves

// Idle times (see power.h), each a step in the same fixed list. Sleep blanks the screen and any key wakes it;
// deep sleep powers almost everything down and only BtnGO wakes it. Deep sleep is never earlier than sleep:
// raising sleep past it pushes it up too, and it can't be lowered below sleep.
int timeoutCount();
int sleepLevel();
void sleepSet(int level);         // clamps and saves
const char *sleepLabel();         // e.g. "30s", "never"
unsigned sleepSeconds();          // 0 means never
int deepSleepLevel();
void deepSleepSet(int level);
const char *deepSleepLabel();
unsigned deepSleepSeconds();

// Time zone as an offset from UTC, in 30 minute steps from UTC-12:00 to UTC+14:00 (see clock.h).
int timezoneLevel();
void timezoneSet(int level);      // clamps and saves
const char *timezoneLabel();      // e.g. "UTC", "UTC+2", "UTC+5:30"
int timezoneMinutes();            // offset from UTC, negative west of it

// Power saving: Wi-Fi is off (and can't be switched on), the clock is hidden and not synced, and the idle sleep screen is dark
// instead of showing the clock (see power.h). Off by default.
bool powerSaving();
void powerSavingSet(bool on); // saves; the caller applies it to the radio with wifiApply() (wifi.h)

// Wi-Fi radio on or off (see wifi.h for switching it; this is only what is saved). Off by default.
bool wifiOn();
void wifiOnSet(bool on); // saves

// RGB LED on the front, as a brightness step from LED_MIN (off) to LED_MAX and one of a short list of colors.
static constexpr int LED_MIN = 0;
static constexpr int LED_MAX = 10;
int ledLevel();
void ledSet(int level); // clamps, applies to the LED and saves
int ledColorCount();
int ledColorLevel();
void ledColorSet(int level);   // wraps round, applies to the LED and saves
const char *ledColorLabel();   // e.g. "red"
uint32_t ledColorRgb();        // 0xRRGGBB of the selected color, to tint the label
void ledOff();                 // lights out without touching the saved setting (the LED keeps its last color through deep sleep)

// Color theme, an index into the THEMES table (see theme.h). Switching it also switches the LED color to the theme's.
int themeLevel();
void themeSet(int level); // wraps round, applies the theme's LED color and saves
