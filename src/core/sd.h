#pragma once

// The microSD card, on the Cardputer's own SPI pins. Paths start at "/" (the Arduino SD library).
// A missing or broken card is never fatal: it just reads as "no card" until sdMount() finds one.

// Call once in setup(): starts the SPI bus and tries to mount the card.
void sdBegin();

// True if a card is mounted right now. Unlike sdMount() it never tries to mount, which can take a moment with no card in.
bool sdMounted();

// Mounts the card if it is not mounted yet (it may have been put in later). True if a card is ready.
bool sdMount();

// Call after a read failed: the card was pulled or is broken. sdMount() looks again.
void sdUnmount();

// "no card", or the card's size for the settings row, e.g. "29.7G" or "512M".
const char *sdLabel();
