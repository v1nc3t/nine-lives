#include "boot.h"

#include <AnimatedGIF.h>
#include <M5Cardputer.h>

// boot.gif is linked into flash by `board_build.embed_files` in platformio.ini
extern const uint8_t boot_gif_start[] asm("_binary_assets_boot_gif_start");
extern const uint8_t boot_gif_end[] asm("_binary_assets_boot_gif_end");

static constexpr uint32_t BOOT_MS = 5000;

static AnimatedGIF gif;

// Called by AnimatedGIF for every decoded line: palette indices -> RGB565 -> screen
static void gifDrawLine(GIFDRAW *d) {
    static uint16_t line[240];
    int width = d->iWidth > 240 ? 240 : d->iWidth;
    int y = d->iY + d->y;
    const uint8_t *src = d->pPixels;

    if (!d->ucHasTransparency) {
        for (int x = 0; x < width; x++) line[x] = d->pPalette[src[x]];
        M5Cardputer.Display.pushImage(d->iX, y, width, 1, line);
        return;
    }

    // Frames only store what changed; skip transparent pixels so the previous frame shows through
    int x = 0;
    while (x < width) {
        while (x < width && src[x] == d->ucTransparent) x++;
        int start = x;
        while (x < width && src[x] != d->ucTransparent) {
            line[x] = d->pPalette[src[x]];
            x++;
        }
        if (x > start) M5Cardputer.Display.pushImage(d->iX + start, y, x - start, 1, &line[start]);
    }
}

// Plays the boot animation once and returns after BOOT_MS: cut off if longer, last frame held if shorter
void playBootAnimation() {
    auto &lcd = M5Cardputer.Display;
    lcd.fillScreen(TFT_BLACK);
    // M5GFX reads a uint16_t* as byte-swapped RGB565, so ask the decoder for big-endian colors
    gif.begin(GIF_PALETTE_RGB565_BE);
    uint32_t start = millis();
    if (gif.open(const_cast<uint8_t *>(boot_gif_start), boot_gif_end - boot_gif_start, gifDrawLine)) {
        lcd.startWrite();
        while (millis() - start < BOOT_MS && gif.playFrame(true, nullptr)) {
        }
        lcd.endWrite();
        gif.close();
    }
    while (millis() - start < BOOT_MS) delay(10);
    lcd.fillScreen(TFT_BLACK);
}
