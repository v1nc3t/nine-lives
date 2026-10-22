#include "sd.h"

#include <SD.h>
#include <SPI.h>
#include <cstring>

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

bool sdMounted() { return mounted; }

bool sdMount() {
    if (!mounted) mounted = SD.begin(SD_CS_PIN, SPI, SD_HZ);
    return mounted;
}

void sdUnmount() {
    SD.end();
    mounted = false;
}

bool sdRemove(const char *path) {
    if (!path[1]) return false;
    char at[128];
    if (snprintf(at, sizeof(at), "%s", path) >= (int)sizeof(at)) return false;

    bool dir;
    {
        File top = SD.open(at);
        if (!top) return false;
        dir = top.isDirectory();
        top.close();
    }
    if (!dir) return SD.remove(at);

    // Walks down without recursion (the loop task's stack is small): removes the first thing in the folder `at`, goes into it if it is a
    // folder, and removes the folder itself once it is empty, then steps back up.
    size_t base = strlen(at);
    while (true) {
        char child[sizeof(at)];
        bool empty;
        bool childIsDir = false;
        {
            File folder = SD.open(at);
            if (!folder) return false;
            File first = folder.openNextFile();
            empty = !first;
            if (!empty) {
                if (snprintf(child, sizeof(child), "%s/%s", at, first.name()) >= (int)sizeof(child)) return false;
                childIsDir = first.isDirectory();
                first.close();
            }
            folder.close();
        }
        if (empty) {
            if (!SD.rmdir(at)) return false;
            if (strlen(at) == base) return true;
            *strrchr(at, '/') = 0;
        } else if (childIsDir) {
            memcpy(at, child, sizeof(at));
        } else if (!SD.remove(child)) {
            return false;
        }
    }
}

const char *sdLabel() {
    static char label[8];
    if (!mounted) return "no card";
    uint64_t mb = SD.cardSize() / (1024 * 1024);
    if (mb >= 1024) snprintf(label, sizeof(label), "%.1fG", mb / 1024.0);
    else snprintf(label, sizeof(label), "%uM", (unsigned)mb);
    return label;
}
