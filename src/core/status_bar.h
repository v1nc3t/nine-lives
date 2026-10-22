#pragma once

// Top strip: clock on the left; on the right the Bluetooth icon and the Wi-Fi icon (grey and crossed out when off, white when on, green when joined / connected) and the battery level (red "LOW" when nearly empty and not charging).
static constexpr int STATUS_BAR_H = 14;

// Draws the whole bar inside its own rectangle.
void statusBarDraw();

// Call every loop(); redraws only when the minute, the battery level or the Wi-Fi or Bluetooth state changed (checked every second).
void statusBarUpdate();
