#pragma once

// The SD card's files, a page on its own (not on the page bar): opened from the sd card row in settings, which main.cpp runs like any page.
// Under the status bar: a bar with the folder you are in, then the back row ('<', with the keys d: delete and r: rename on the right; it stays put),
// then the folders and the files, folders first.
// Up/down (fn + ; and .) move, Enter or BtnGO opens a folder, or a .txt, .md or .conf file in a preview popup (other files are grey and don't open).
// D asks to delete the selected file or folder (with everything in it) in a popup: Enter deletes, ESC or Backspace cancels; BtnGO does nothing there.
// R renames it in a popup with the old name to edit (see text_input.h for the keys): Enter saves, fn + ` cancels; a name that is taken is refused.
// Backspace and ESC go up a folder, and from the top one back to settings.
// The preview popup shows the first 4 KB of the file, wrapped: up/down (fn + ; and .) scroll a line, left/right (fn + , and /) a screen,
// Enter, BtnGO, Backspace or ESC close it.
// A missing or broken card shows a message instead; Enter looks again.
void sdBrowserOpen(); // lists the top folder and opens the page
