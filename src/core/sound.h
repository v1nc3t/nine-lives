#pragma once

// Short sounds on the speaker, played in the background (a sound is a few notes, one after the other).
// Add a sound in the SOUNDS table at the top of sound.cpp; it shows up in settings by itself.
// Nothing plays while sound is switched off in settings (see settings.h's soundOn()).

int soundCount();
const char *soundName(int index);

void soundPlay(int index); // starts the sound, cutting off one that is still playing
void soundUpdate();        // every loop(): plays the next note when it is time
