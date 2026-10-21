#include "sound.h"

#include <M5Cardputer.h>
#include <cstddef>

#include "settings.h"

struct Note {
    uint16_t hz; // 0 is a rest
    uint16_t ms;
};

struct Sound {
    const char *name; // shown in settings
    const Note *notes;
    uint8_t count;
};

template <size_t N> static constexpr Sound sound(const char *name, const Note (&notes)[N]) { return {name, notes, N}; }

static const Note CLICK_NOTES[] = {{1500, 25}};
static const Note BEEP_NOTES[] = {{880, 100}};
static const Note CHIME_NOTES[] = {{523, 120}, {659, 120}, {784, 120}, {1047, 240}};
static const Note ALARM_NOTES[] = {{880, 120}, {660, 120}, {880, 120}, {660, 120}, {880, 120}, {660, 120}};
static const Note HAPPY_NOTES[] = {{659, 90}, {880, 90}, {1319, 200}};
static const Note ERROR_NOTES[] = {{330, 160}, {196, 320}};
static const Note MEOW_NOTES[] = {{500, 40}, {600, 40}, {750, 40}, {900, 60}, {800, 40}, {650, 40}, {520, 40}, {420, 60}};

// SOUNDS: add a line here to add a sound. The first one is the default.
static const Sound SOUNDS[] = {
    sound("click", CLICK_NOTES), sound("beep", BEEP_NOTES),   sound("chime", CHIME_NOTES), sound("alarm", ALARM_NOTES),
    sound("happy", HAPPY_NOTES), sound("error", ERROR_NOTES), sound("meow", MEOW_NOTES),
};
static constexpr int SOUND_COUNT = sizeof(SOUNDS) / sizeof(SOUNDS[0]);

static const Sound *playing = nullptr; // nullptr when nothing is playing
static int nextNote;
static uint32_t nextAt; // when the note that is sounding now ends

int soundCount() { return SOUND_COUNT; }
static int soundClamp(int index) { return constrain(index, 0, SOUND_COUNT - 1); }
const char *soundName(int index) { return SOUNDS[soundClamp(index)].name; }

void soundPlay(int index) {
    if (!soundOn()) return;
    playing = &SOUNDS[soundClamp(index)];
    nextNote = 0;
    nextAt = millis();
}

void soundUpdate() {
    if (!playing) return;
    if (!soundOn()) { // switched off while playing
        M5Cardputer.Speaker.stop();
        playing = nullptr;
        return;
    }
    if ((int32_t)(millis() - nextAt) < 0) return;
    if (nextNote == playing->count) {
        playing = nullptr;
        return;
    }
    const Note &note = playing->notes[nextNote++];
    if (note.hz) M5Cardputer.Speaker.tone(note.hz, note.ms);
    nextAt = millis() + note.ms;
}
