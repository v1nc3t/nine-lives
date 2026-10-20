#include "page.h"
#include "theme.h"

#include "settings_page.h"

// Empty page with just the back row; a stand-in until the real page exists.
static void blankPageDraw() { pageHeaderDraw(); }
static bool blankPageHandleKey(const Keyboard_Class::KeysState &keys) { return pageHandleBack(keys); }

// To build a page: write draw/handleKey for it and swap them in for the blank ones.
static const Page PAGES[] = {
    {'0', "groom", "settings", settingsPageDraw, settingsPageHandleKey, settingsPageHandleButton, settingsPageUpdate},
    {'1', "hoard", "vault", blankPageDraw, blankPageHandleKey},
    {'2', "whiskers", "2FA", blankPageDraw, blankPageHandleKey},
    {'3', "scratchpad", "notes", blankPageDraw, blankPageHandleKey},
    {'4', "purr", "voice memos", blankPageDraw, blankPageHandleKey},
    {'5', "pawprints", "habbits and checklists", blankPageDraw, blankPageHandleKey},
    {'6', "pounce", "focus timer", blankPageDraw, blankPageHandleKey},
    {'7', "kibble", "catagotchi", blankPageDraw, blankPageHandleKey},
    {'8', "yowl", "messenger", blankPageDraw, blankPageHandleKey},
    {'9', "meow", "pager", blankPageDraw, blankPageHandleKey},
};

const Page *pageFor(char label) {
    for (const Page &p : PAGES) {
        if (p.label == label) return &p;
    }
    return nullptr;
}

static const Page *opened = nullptr;

void pageOpen(const Page *page) { opened = page; }
void pageClose() { opened = nullptr; }
const Page *pageOpened() { return opened; }

void pageFrameDraw(bool pageBarVisible) {
    auto &lcd = M5Cardputer.Display;
    int w = lcd.width();
    int h = lcd.height() - STATUS_BAR_H - (pageBarVisible ? PAGE_BAR_H : 0);

    lcd.fillRect(0, STATUS_BAR_H, w, h, TFT_BLACK);
    lcd.drawRect(0, STATUS_BAR_H, w, h, themeColor());
}

void pageFit(char *out, const char *text, int max) {
    if ((int)strlen(text) <= max) {
        strcpy(out, text);
    } else {
        memcpy(out, text, max - 2);
        strcpy(out + max - 2, "..");
    }
}

void pageScrollBarDraw(int top, int visible, int count) {
    if (count <= visible) return;
    auto &lcd = M5Cardputer.Display;
    int trackY = STATUS_BAR_H + 2;
    int trackH = lcd.height() - STATUS_BAR_H - 4;
    int thumbH = trackH * visible / count;
    int thumbY = trackY + (trackH - thumbH) * top / (count - visible);
    lcd.fillRect(lcd.width() - 3, trackY, 2, trackH, TFT_DARKGREY);
    lcd.fillRect(lcd.width() - 3, thumbY, 2, thumbH, themeColor());
}

void pageHeaderDraw() {
    pageFrameDraw();

    auto &lcd = M5Cardputer.Display;
    lcd.setTextDatum(middle_left);
    lcd.setTextSize(1.5);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.drawString("<", 8, STATUS_BAR_H + PAGE_ROW_H / 2);
}

bool pageEscPressed(const Keyboard_Class::KeysState &keys) {
    for (char c : keys.word) {
        if (c == '`') return true;  // the ESC key
    }
    return false;
}

bool pageHandleBack(const Keyboard_Class::KeysState &keys) {
    if (!keys.enter && !keys.del && !pageEscPressed(keys)) return false;
    pageClose();
    return true;
}
