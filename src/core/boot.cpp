#include "boot.h"

#include <AnimatedGIF.h>
#include <M5Cardputer.h>

// boot.gif is linked into flash by `board_build.embed_files` in platformio.ini
extern const uint8_t boot_gif_start[] asm("_binary_assets_boot_gif_start");
extern const uint8_t boot_gif_end[] asm("_binary_assets_boot_gif_end");

static constexpr uint32_t BOOT_MS = 5000;

static AnimatedGIF gif;
static M5Canvas canvas(&M5Cardputer.Display); // whole frame is composed here, then pushed to the LCD in one go (no tearing)

// Called by AnimatedGIF for every decoded line: palette indices -> canvas (RGB565, already in the LCD's byte order)
static void gifDrawLine(GIFDRAW *d) {
    int y = d->iY + d->y;
    if (y >= canvas.height() || d->iX >= canvas.width()) return;
    int width = min<int>(d->iWidth, canvas.width() - d->iX);
    auto *dst = static_cast<uint16_t *>(canvas.getBuffer()) + y * canvas.width() + d->iX;
    const uint8_t *src = d->pPixels;

    // Frames only store what changed; skip transparent pixels so the previous frame shows through
    for (int x = 0; x < width; x++) {
        if (!d->ucHasTransparency || src[x] != d->ucTransparent) dst[x] = d->pPalette[src[x]];
    }
}

// True when a key (or the GO button) was pressed since the last call. Polls the inputs itself, as loop() is not running yet.
static bool skipRequested() {
    M5Cardputer.update();
    bool keyPressed = M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed(); // stateful: call once per poll
    return keyPressed || M5Cardputer.BtnA.wasPressed();
}

// Plays the boot animation once and returns after BOOT_MS: cut off if longer, last frame held if shorter.
// Any key press ends it early.
void playBootAnimation() {
    auto &lcd = M5Cardputer.Display;
    lcd.fillScreen(TFT_BLACK);
    canvas.setColorDepth(16);
    if (!canvas.createSprite(lcd.width(), lcd.height())) {
        delay(BOOT_MS); // not enough RAM for the canvas: skip the animation
        return;
    }
    canvas.fillSprite(TFT_BLACK);

    // M5GFX reads a uint16_t* as byte-swapped RGB565, so ask the decoder for big-endian colors
    gif.begin(GIF_PALETTE_RGB565_BE);
    uint32_t start = millis();
    bool skipped = false;
    if (gif.open(const_cast<uint8_t *>(boot_gif_start), boot_gif_end - boot_gif_start, gifDrawLine)) {
        uint32_t next = start; // when the next frame is due; decode time counts towards the frame delay
        int delayMs = 0;
        while (!skipped && millis() - start < BOOT_MS) {
            bool more = gif.playFrame(false, &delayMs);
            canvas.pushSprite(0, 0);
            if (!more) break;
            next += delayMs;
            while (!skipped && millis() < next && millis() - start < BOOT_MS) {
                skipped = skipRequested();
                delay(1);
            }
        }
        gif.close();
    }
    canvas.deleteSprite();
    while (!skipped && millis() - start < BOOT_MS) {
        skipped = skipRequested();
        delay(10);
    }
    lcd.fillScreen(TFT_BLACK);
}
