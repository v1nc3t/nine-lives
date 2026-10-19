#pragma once

// Top strip: clock on the left, battery level on the right (red "LOW" when nearly empty and not charging).
static constexpr int STATUS_BAR_H = 14;

// Draws the whole bar inside its own rectangle.
void statusBarDraw();

// Call every loop(); redraws only when the minute or the battery level changed (checked every second).
void statusBarUpdate();
