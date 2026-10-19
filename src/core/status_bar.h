#pragma once

// Top strip: clock on the left (hidden while power saving is on); on the right the Wi-Fi icon (grey and crossed out when off, white when on, green when joined) and the battery level (red "LOW" when nearly empty and not charging).
static constexpr int STATUS_BAR_H = 14;

// Draws the whole bar inside its own rectangle.
void statusBarDraw();

// Call every loop(); redraws only when the minute, the battery level or the Wi-Fi state changed (checked every second).
void statusBarUpdate();
