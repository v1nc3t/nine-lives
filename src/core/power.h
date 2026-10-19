#pragma once

// Idle power saving, with two stages set in settings (both count from the last key or button press):
//  - sleep: the screen shows the clock, big and dim, and the CPU slows down; any key or BtnGO wakes it and carries on where it was.
//    With power saving on (see settings.h) the screen stays dark instead and the chip light-sleeps between key checks.
//  - deep sleep: the chip powers down (almost no battery use). Only BtnGO (the button on the side) wakes it;
//    a wake is a full reboot, so everything starts fresh (open page, unlocked keys) - but see powerWokeFromSleep().
//    Wi-Fi is switched off and the time is thrown away first (the next start uses the build time until it is set or synced).

// Call on every key or button press.
void powerActivity();

// Call every loop(). Sleeps once idle for the sleep time and deep-sleeps once idle for the deep sleep time.
// Returns true after waking from sleep, when the caller must redraw the screen. Does not return when it deep-sleeps.
bool powerUpdate();

// True if this boot is a wake from deep sleep (setup() skips the boot animation then).
bool powerWokeFromSleep();
