#pragma once

// The SD card's files, a page on its own (not on the page bar): opened from the sd card row in settings, which main.cpp runs like any page.
// Rows: back ('<', with the folder on the right), then the folders and the files, folders first.
// Up/down (fn + ; and .) move, Enter or BtnGO opens a folder, or a .txt or .md file in a preview popup (other files are grey and don't open).
// Backspace and ESC go up a folder, and from the top one back to settings.
// The preview popup shows the first 4 KB of the file, wrapped: up/down (fn + ; and .) scroll a line, left/right (fn + , and /) a screen,
// Enter, BtnGO, Backspace or ESC close it.
// A missing or broken card shows a message instead; Enter looks again.
void sdBrowserOpen(); // lists the top folder and opens the page
