#pragma once

// The user's settings as a text file on the SD card, "/nine-lives.conf": one "key=value" line per setting, the value a whole number
// (the step shown in settings, e.g. timezone=24 is UTC and each step is 30 minutes). Lines without '=' or starting with '#' are ignored.
// The card is the master: at boot the file, if there is one, replaces the settings kept in flash; after that every change is written to it.
// A change made while there is no card stays in flash only, and is lost if a card with an older file is in at the next boot.
// Wi-Fi passwords and the paired phone are not in it. A new setting is added to the SETTINGS table in config.cpp.

// Call once in setup() after sdBegin() and settingsLoad(): applies the file, or creates it from the current settings when the card has none.
void configLoad();

// Writes the file. settings.cpp calls it on every change; does nothing without a card, or while the file is being read.
void configSave();
