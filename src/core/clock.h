#pragma once

// Wall clock. The RTC chip (or, without one, the ESP32 system clock) keeps UTC; the time zone from settings is added when showing it.
// The time is set by hand in settings, by itself over Wi-Fi (NTP) whenever the device is connected, or by an iPhone each time it connects over Bluetooth
// (see bluetooth.h); a stale RTC is set to the firmware's build time.

// Call once in setup() after M5Cardputer.begin().
void clockBegin();

// Call every loop(): while Wi-Fi is connected, fetches the time from the internet (once on connecting, then the network time
// service repeats it every hour) and sets the clock; does the same with the time the phone sent over Bluetooth.
// The time zone from settings is untouched: the phone's local time is turned into UTC with it, so it must be right for the clock to match the phone.
void clockSyncUpdate();

// Local time as "HH:MM". Returns a pointer to a static buffer.
const char *clockText();

// Local time as minutes since midnight (0..1439).
int clockLocalMinutes();

// Sets the local time of day (seconds go to 0); the date and time zone stay as they are.
void clockSet(int hours, int minutes);
