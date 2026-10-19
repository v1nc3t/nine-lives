#pragma once

// User settings, kept in the chip's flash (NVS) so they survive a reboot.

// Call once in setup() after M5Cardputer.begin(): applies the saved settings.
void settingsLoad();

// Screen brightness as a step from BRIGHTNESS_MIN to BRIGHTNESS_MAX (never fully off, so the screen can't be lost).
static constexpr int BRIGHTNESS_MIN = 1;
static constexpr int BRIGHTNESS_MAX = 10;
int brightnessLevel();
void brightnessSet(int level); // clamps, applies to the screen and saves

// Idle time before the screen goes blank and the device deep-sleeps (see power.h). A step in a fixed list.
int timeoutCount();
int timeoutLevel();
void timeoutSet(int level);       // clamps and saves
const char *timeoutLabel();       // e.g. "30s", "never"
unsigned timeoutSeconds();        // 0 means never
