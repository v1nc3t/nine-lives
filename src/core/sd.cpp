#include "sd.h"

#include <SD.h>
#include <SPI.h>

// The Cardputer's microSD slot
static constexpr int SD_SCK_PIN = 40;
static constexpr int SD_MISO_PIN = 39;
static constexpr int SD_MOSI_PIN = 14;
static constexpr int SD_CS_PIN = 12;
static constexpr uint32_t SD_HZ = 25000000;

static bool mounted = false;

void sdBegin() {
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    sdMount();
}

bool sdMount() {
    if (!mounted) mounted = SD.begin(SD_CS_PIN, SPI, SD_HZ);
    return mounted;
}

void sdUnmount() {
    SD.end();
    mounted = false;
}

const char *sdLabel() {
    static char label[8];
    if (!mounted) return "no card";
    uint64_t mb = SD.cardSize() / (1024 * 1024);
    if (mb >= 1024) snprintf(label, sizeof(label), "%.1fG", mb / 1024.0);
    else snprintf(label, sizeof(label), "%uM", (unsigned)mb);
    return label;
}
