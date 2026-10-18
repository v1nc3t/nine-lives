#pragma once

// Bottom strip: 1234567890, the selected page in bold white.
static constexpr int PAGE_BAR_H = 16;

// Draws the whole bar inside its own rectangle.
void pageBarDraw();

// A digit selects it; ',' (left arrow) moves to the previous one, '/' (right arrow) to the next (both wrap around).
// Returns true if the selection changed, so the caller knows to redraw.
bool pageBarHandleKey(char c);

// The selected page's digit.
char pageBarSelectedLabel();
