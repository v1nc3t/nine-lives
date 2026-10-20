#include "sd_browser.h"
#include "theme.h"

#include <SD.h>
#include <strings.h>

#include "page.h"
#include "sd.h"

static constexpr int MAX_ENTRIES = 64; // shortcut: a folder shows its first 64 entries (a note row says so), page them in when a folder needs more
static constexpr int NAME_LEN = 64;    // longer names are cut, and then can't be opened
static constexpr int PATH_LEN = 128;   // deeper folders can't be entered

// Preview popup: the same box as the Wi-Fi popup, text in the small 6 px font
static constexpr int POPUP_X = 10;
static constexpr int POPUP_Y = STATUS_BAR_H + 5;
static constexpr int POPUP_W = 220;
static constexpr int POPUP_H = 110;
static constexpr int TITLE_MAX_CHARS = 20;
static constexpr int TEXT_X = POPUP_X + 8;
static constexpr int TEXT_Y = POPUP_Y + 26;
static constexpr int LINE_H = 10;
static constexpr int COLS = (POPUP_W - 16) / 6;
static constexpr int ROWS = (POPUP_H - 26 - 4) / LINE_H; // under the title, down to the border
static constexpr int PREVIEW_MAX = 4096; // shortcut: only the first 4 KB of a file, raise it or read in pieces when previews need more

struct FileEntry {
    char name[NAME_LEN];
    bool dir;
};

static FileEntry entries[MAX_ENTRIES];
static int count = 0;
static bool more = false;   // the folder has more entries than MAX_ENTRIES
static bool failed = false; // the card is missing or can't be read
static char path[PATH_LEN] = "/";
static int selected = 0; // row: 0 is the back row, then the entries
static int top = 0;      // first row shown

static bool previewing = false;
static bool modalOnly = false; // only the preview changed since the last draw: repaint just it, so nothing flickers
static char previewName[NAME_LEN];
static char previewText[PREVIEW_MAX + 4]; // room for the "\n..." that ends a cut file
static int previewLen = 0;
static int previewLines = 0;
static int scroll = 0; // first line shown

static bool supported(const char *name) {
    const char *dot = strrchr(name, '.');
    return dot && (!strcasecmp(dot, ".txt") || !strcasecmp(dot, ".md"));
}

// Folders first, then by name
static bool before(const FileEntry &a, const FileEntry &b) { return a.dir != b.dir ? a.dir : strcasecmp(a.name, b.name) < 0; }

static void entryAdd(const char *name, bool dir) {
    if (count == MAX_ENTRIES) {
        more = true;
        return;
    }
    FileEntry entry;
    snprintf(entry.name, NAME_LEN, "%s", name);
    entry.dir = dir;
    int i = count++;
    for (; i > 0 && before(entry, entries[i - 1]); i--) entries[i] = entries[i - 1];
    entries[i] = entry;
}

// Lists dir, mounting the card first if needed. A card that is missing or can't be read sets `failed`.
static void listOpen(const char *dir) {
    snprintf(path, PATH_LEN, "%s", dir);
    count = 0;
    more = false;
    selected = 0;
    top = 0;
    failed = !sdMount();
    if (failed) return;

    File folder = SD.open(path);
    failed = !folder || !folder.isDirectory();
    if (failed) {
        sdUnmount();
        return;
    }
    for (File f = folder.openNextFile(); f; f = folder.openNextFile()) {
        if (f.name()[0] != '.') entryAdd(f.name(), f.isDirectory()); // dot files are hidden
    }
}

// path + name; false if it doesn't fit
static bool pathJoin(char *out, const char *name) { return snprintf(out, PATH_LEN, "%s%s%s", path, path[1] ? "/" : "", name) < PATH_LEN; }

// The back row, Backspace or ESC: up one folder, and from the top one (or a card that can't be read) back to settings
static bool up() {
    if (failed || !path[1]) {
        pageOpen(pageFor('0'));
        return true;
    }
    char parent[PATH_LEN];
    snprintf(parent, PATH_LEN, "%s", path);
    char *slash = strrchr(parent, '/');
    if (slash == parent) slash++; // "/a" goes to "/"
    *slash = 0;
    listOpen(parent);
    return true;
}

// Copies the next wrapped line (up to COLS characters) of the preview text; false at the end.
static bool lineNext(const char *&p, const char *end, char *line) {
    if (p >= end) return false;
    int n = 0;
    while (p < end && n < COLS) {
        unsigned char c = *p++;
        if (c == '\n') {
            line[n] = 0;
            return true;
        }
        if (c == '\r' || (c & 0xC0) == 0x80) continue; // CR, UTF-8 continuation bytes
        line[n++] = c < ' ' ? ' ' : c < 0x7F ? c : '?'; // the font has only ASCII
    }
    while (p < end && *p == '\r') p++;
    if (p < end && *p == '\n') p++; // a line that just fills the width: its newline doesn't start an empty one
    line[n] = 0;
    return true;
}

static bool previewOpen(const FileEntry &entry) {
    char file[PATH_LEN];
    if (!pathJoin(file, entry.name)) return false;

    File f = SD.open(file);
    size_t size = f ? f.size() : 0;
    previewLen = size ? f.read((uint8_t *)previewText, min<size_t>(size, PREVIEW_MAX)) : 0;
    if (!f || (size && !previewLen)) { // the card went away or is broken
        sdUnmount();
        failed = true;
        return true;
    }
    if (size > PREVIEW_MAX) {
        memcpy(previewText + previewLen, "\n...", 4);
        previewLen += 4;
    } else if (previewLen == 0) {
        memcpy(previewText, "(empty)", 7);
        previewLen = 7;
    }

    char line[COLS + 1];
    const char *p = previewText;
    previewLines = 0;
    while (lineNext(p, previewText + previewLen, line)) previewLines++;

    snprintf(previewName, NAME_LEN, "%s", entry.name);
    scroll = 0;
    modalOnly = false;
    previewing = true;
    return true;
}

// Enter or BtnGO
static bool activate() {
    if (previewing) {
        previewing = false;
        return true;
    }
    if (failed) {
        listOpen("/");
        return true;
    }
    if (selected == 0) return up();
    if (selected > count) return false; // the note row

    const FileEntry &entry = entries[selected - 1];
    if (!entry.dir) return supported(entry.name) && previewOpen(entry);
    char folder[PATH_LEN];
    if (!pathJoin(folder, entry.name)) return false;
    listOpen(folder);
    return true;
}

// Row after the entries, saying why the list ends or is empty (nullptr: none)
static const char *noteText() { return more ? "more files not shown" : count == 0 ? "empty" : nullptr; }
static int rowCount() { return 1 + count + (noteText() != nullptr); }
static int visibleRows() { return (M5Cardputer.Display.height() - STATUS_BAR_H) / PAGE_ROW_H; }
static int rowY(int row) { return STATUS_BAR_H + (row - top) * PAGE_ROW_H; }

static void listDraw() {
    auto &lcd = M5Cardputer.Display;
    if (selected < top) top = selected;
    if (selected >= top + visibleRows()) top = selected - visibleRows() + 1;

    pageFrameDraw();
    lcd.setTextSize(1.5);
    int nameMax = (lcd.width() - 16) / lcd.textWidth("0");
    for (int row = top; row < rowCount() && row < top + visibleRows(); row++) {
        int y = rowY(row) + PAGE_ROW_H / 2;
        char full[NAME_LEN + 2];
        char shown[NAME_LEN + 2];
        if (row == 0) {
            lcd.setTextDatum(middle_left);
            lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            lcd.drawString("<", 8, y);
            pageFit(shown, path, nameMax - 2);
            lcd.setTextDatum(middle_right);
            lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            lcd.drawString(shown, lcd.width() - 8, y);
        } else if (row > count) {
            lcd.setTextDatum(middle_left);
            lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
            lcd.drawString(noteText(), 8, y);
        } else {
            const FileEntry &entry = entries[row - 1];
            snprintf(full, sizeof(full), entry.dir ? "%s/" : "%s", entry.name);
            pageFit(shown, full, nameMax);
            lcd.setTextDatum(middle_left);
            lcd.setTextColor(entry.dir ? themeColor() : supported(entry.name) ? TFT_WHITE : TFT_DARKGREY, TFT_BLACK);
            lcd.drawString(shown, 8, y);
        }
    }
    pageScrollBarDraw(top, visibleRows(), rowCount());

    // Box around the selected row
    lcd.drawRect(3, rowY(selected) + 1, lcd.width() - 8, PAGE_ROW_H - 2, themeColor());
}

static void failedDraw() {
    pageHeaderDraw();
    auto &lcd = M5Cardputer.Display;
    int x = lcd.width() / 2;
    int y = PAGE_CONTENT_Y + (lcd.height() - PAGE_CONTENT_Y) / 2;
    lcd.setTextDatum(middle_center);
    lcd.setTextSize(1.5);
    lcd.setTextColor(TFT_RED, TFT_BLACK);
    lcd.drawString("card missing or broken", x, y - 8);
    lcd.setTextSize(1);
    lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    lcd.drawString("enter: look again   esc: back", x, y + 10);
}

static void previewDraw() {
    auto &lcd = M5Cardputer.Display;
    lcd.fillRect(POPUP_X, POPUP_Y, POPUP_W, POPUP_H, TFT_BLACK);
    lcd.drawRect(POPUP_X, POPUP_Y, POPUP_W, POPUP_H, themeColor());

    char title[TITLE_MAX_CHARS + 1];
    pageFit(title, previewName, TITLE_MAX_CHARS);
    lcd.setTextDatum(middle_center);
    lcd.setTextSize(1.5);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.drawString(title, POPUP_X + POPUP_W / 2, POPUP_Y + 11);
    lcd.drawFastHLine(POPUP_X + 4, POPUP_Y + 21, POPUP_W - 8, TFT_DARKGREY);

    lcd.setTextSize(1);
    lcd.setTextDatum(top_left);
    char line[COLS + 1];
    const char *p = previewText;
    const char *end = previewText + previewLen;
    for (int i = 0; i < scroll; i++) lineNext(p, end, line);
    for (int i = 0; i < ROWS && lineNext(p, end, line); i++) lcd.drawString(line, TEXT_X, TEXT_Y + i * LINE_H);

    lcd.setTextDatum(middle_center);
    lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
}

static void draw() {
    if (previewing && modalOnly) {
        modalOnly = false;
        previewDraw();
        return;
    }
    if (failed) failedDraw();
    else listDraw();
    if (previewing) previewDraw();
}

// Moves the preview with ; . (a line) and , / (a screen)
static bool previewMove(char c) {
    int step = c == ';' ? -1 : c == '.' ? 1 : c == ',' ? -ROWS : c == '/' ? ROWS : 0;
    int to = constrain(scroll + step, 0, max(0, previewLines - ROWS));
    if (to == scroll) return false;
    scroll = to;
    modalOnly = true;
    return true;
}

static bool handleKey(const Keyboard_Class::KeysState &keys) {
    if (keys.enter) return activate();
    if (keys.del || pageEscPressed(keys)) {
        if (!previewing) return up();
        previewing = false;
        return true;
    }

    bool changed = false;
    for (char c : keys.word) {
        if (previewing) {
            changed |= previewMove(c);
        } else if (c == ';') {  // up arrow (fn key)
            selected = (selected + rowCount() - 1) % rowCount();
            changed = true;
        } else if (c == '.') {  // down arrow (fn key)
            selected = (selected + 1) % rowCount();
            changed = true;
        }
    }
    return changed;
}

static bool handleButton() { return activate(); }

static const Page PAGE = {0, "sd card", "files", draw, handleKey, handleButton}; // no page bar digit: settings opens it

void sdBrowserOpen() {
    previewing = false;
    listOpen("/");
    pageOpen(&PAGE);
}
