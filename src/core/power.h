#pragma once

// Idle sleep. When nothing was pressed for the timeout set in settings, the screen is switched off
// and the chip goes into deep sleep (almost no battery use). BtnGO (the button on the side) wakes it;
// a wake is a full reboot, so everything starts fresh (open page, unlocked keys) - but see wokeFromSleep().

// Call on every key or button press.
void powerActivity();

// Call every loop(); goes to sleep once idle for the timeout. Does not return when it sleeps.
void powerUpdate();

// True if this boot is a wake from deep sleep (setup() skips the boot animation then).
bool powerWokeFromSleep();
