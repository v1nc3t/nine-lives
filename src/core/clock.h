#pragma once

// Wall clock. The RTC chip (or, without one, the ESP32 system clock) keeps UTC; the time zone from settings is added when showing it.
// Until Wi-Fi or phone sync exists (see PLAN.md) the time is set by hand in settings; a stale RTC is set to the firmware's build time.

// Call once in setup() after M5Cardputer.begin().
void clockBegin();

// Local time as "HH:MM". Returns a pointer to a static buffer.
const char *clockText();

// Local time as minutes since midnight (0..1439).
int clockLocalMinutes();

// Sets the local time of day (seconds go to 0); the date and time zone stay as they are.
void clockSet(int hours, int minutes);
