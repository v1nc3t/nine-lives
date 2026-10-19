#pragma once

// Plays assets/boot.gif full screen. Returns after 5 seconds (see BOOT_MS in boot.cpp), or as soon as a key is pressed.
// Call once in setup(), after M5Cardputer.begin() and setRotation().
void playBootAnimation();
