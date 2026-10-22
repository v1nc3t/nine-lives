#pragma once

// Idle power saving, with two stages set in settings (both count from the last key or button press):
//  - sleep: the screen shows the clock, big and dim, and the CPU slows down; any key or BtnGO wakes it and carries on where it was.
//    With power saving on (see settings.h) the screen stays dark (its controller asleep too) and the chip light-sleeps between key checks.
//    Either way the speaker and the LED are switched off until it wakes.
//  - deep sleep: the chip powers down (almost no battery use). Only BtnGO (the button on the side) wakes it;
//    a wake is a full reboot, so everything starts fresh (open page, unlocked keys) - but see powerWokeFromSleep().
//    Wi-Fi and Bluetooth are switched off first. The time is kept (it may drift a few minutes a day while asleep; a Wi-Fi sync fixes it).
//    The pins the firmware drives (backlight, speaker, keyboard, LED, screen, SD card, IR LED) are parked at their idle levels and held there,
//    because the chip would otherwise let them float and they could keep drawing current. Of the chip's own parts only BtnGO's
//    wake circuit, the sleep timer and the RTC slow memory the clock needs stay powered.
//    The Cardputer has no switches to cut power to its parts (screen, LED, SD card, speaker amp), so each is put in its lowest
//    power state instead: the screen's controller asleep, the LED black, the amp without a clock, the SD card not selected.

// Call first thing in setup(), before anything drives a pin: lets go of the pins held through deep sleep, and switches the IR LED off.
void powerBegin();

// Holding BtnGO for five seconds deep-sleeps at once, whatever the timers say. Let go of it after the screen goes dark.
// (The press that wakes the chip from deep sleep doesn't count; hold it again after letting go.)

// Call on every key or button press.
void powerActivity();

// Call every loop(). Sleeps once idle for the sleep time and deep-sleeps once idle for the deep sleep time.
// Returns true after waking from sleep, when the caller must redraw the screen. Does not return when it deep-sleeps.
bool powerUpdate();

// True if this boot is a wake from deep sleep (setup() skips the boot animation then).
bool powerWokeFromSleep();
