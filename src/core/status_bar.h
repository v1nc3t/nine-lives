#pragma once

// Top strip: title on the left, battery level on the right.
static constexpr int STATUS_BAR_H = 14;

// Draws the whole bar inside its own rectangle.
void statusBarDraw();

// Call every loop(); redraws only when the battery level changed (checked every few seconds).
void statusBarUpdate();
