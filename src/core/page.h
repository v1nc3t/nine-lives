#pragma once

#include <M5Cardputer.h>

#include "page_bar.h"
#include "status_bar.h"

// A screen that lives on one page bar digit. Add a new one in page.cpp's PAGES table.
// Selecting the digit shows its name on the main screen; Enter or BtnGO opens it.
// An open page owns everything below the status bar (the page bar is hidden).
struct Page {
    char label;                                           // page bar digit it is shown on
    const char *name;                                     // shown on the main screen
    const char *description;                              // one short line, shown under the name on the main screen (max ~20 characters to fit)
    void (*draw)();                                       // paints the whole area between the status bar and the page bar
    bool (*handleKey)(const Keyboard_Class::KeysState &); // returns true if the page needs a redraw
    bool (*handleButton)();                                // BtnGO while the page is open (optional, left out = nullptr); returns true if the page needs a redraw
    bool (*update)();                                      // every loop() while the page is open, for things that happen without a key (optional, left out = nullptr); returns true if the page needs a redraw
};

// The page registered for this digit, or nullptr (the caller falls back to the empty main screen).
const Page *pageFor(char label);

// Opens the page / returns to the main screen. pageOpened() is nullptr on the main screen.
void pageOpen(const Page *page);
void pageClose();
const Page *pageOpened();

// Clears the area and draws the frame in the theme color. The main screen passes true to leave room for the page bar.
void pageFrameDraw(bool pageBarVisible = false);

// The structure every open page shares: frame, then a back row ('<') on the first row, then free content.
// A page's draw() starts with pageHeaderDraw() and draws below PAGE_CONTENT_Y;
// its handleKey() starts with `if (pageHandleBack(keys)) return true;`.
static constexpr int PAGE_ROW_H = 20;
static constexpr int PAGE_CONTENT_Y = STATUS_BAR_H + PAGE_ROW_H;
void pageHeaderDraw();
bool pageEscPressed(const Keyboard_Class::KeysState &keys); // the ESC key (`)
bool pageHandleBack(const Keyboard_Class::KeysState &keys); // Enter, Backspace or ESC closes the page; true if it did
