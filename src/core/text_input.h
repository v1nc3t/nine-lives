#pragma once

#include <M5Cardputer.h>

// Typing text on the keyboard, for any page with a text field.
//  - letters, digits and symbols as printed; shift (or ctrl) gives the upper symbol or a capital
//  - fn + shift toggles caps lock (letters only, digits and symbols are not affected)
//  - fn + , and / move the cursor left and right, fn + ; and . jump to the start and the end
//  - backspace deletes before the cursor, fn + backspace after it
//  - opt shows or hides the text (for passwords)
//  - fn + ` is ESC (a plain ` types a backtick), Enter submits (both are left to the page)

// Call once per loop() with the result of Keyboard.isChange(), before the page handles its keys.
void textInputUpdate(bool keyChanged);

bool textInputCaps();

// What was newly pressed in the last key change. Keys that are still held from before are not repeated,
// so typing with two keys down at once doesn't type the first one twice.
struct TextKeys {
    char chars[8];   // typed characters, caps lock and shift already applied (fn is not: those are in the fields below)
    uint8_t count;
    bool left, right, home, end; // fn + , / ; .
    bool enter, del, forwardDel, opt, esc;
};
const TextKeys &textInputKeys();

static constexpr int TEXT_FIELD_MAX = 63; // the longest Wi-Fi password

struct TextField {
    char text[TEXT_FIELD_MAX + 1];
    uint8_t length;
    uint8_t cursor;
    uint8_t first;  // index of the first character shown
    bool masked;    // shown as '*' (opt toggles it)
};

void textFieldReset(TextField &field, bool masked);

// Applies textInputKeys() to the field; true if the text, the cursor or the mask changed. Enter and ESC are not handled.
bool textFieldHandleKey(TextField &field);

// Draws the box and what is typed in it, with the cursor.
void textFieldDraw(TextField &field, int x, int y, int w, int h);
