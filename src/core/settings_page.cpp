#include "settings_page.h"

#include "page.h"

void settingsPageDraw() {
    pageHeaderDraw();
}

bool settingsPageHandleKey(const Keyboard_Class::KeysState &keys) {
    return pageHandleBack(keys);
}
