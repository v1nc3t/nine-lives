#include "clock.h"

#include <M5Cardputer.h>
#include <esp_sntp.h>
#include <sys/time.h>
#include <time.h>

#include "settings.h"
#include "wifi.h"

#ifndef BUILD_UTC
#define BUILD_UTC 0
#endif

// The original Cardputer has no RTC chip (the Cardputer-Adv has one), so without it the ESP32's own clock is used.
// That one runs through light and deep sleep but is lost when the power goes or the chip resets. In deep sleep it counts
// with the chip's internal RC oscillator, which drifts with temperature (minutes a day); awake it runs off the crystal.
// Each Wi-Fi sync puts it right again.

// The RTC as seconds since 1970, UTC (mktime reads the fields as UTC too, as no time zone is configured)
static time_t rtcSeconds() {
    auto now = M5.Rtc.getDateTime();
    tm rtc = {};
    rtc.tm_year = now.date.year - 1900;
    rtc.tm_mon = now.date.month - 1;
    rtc.tm_mday = now.date.date;
    rtc.tm_hour = now.time.hours;
    rtc.tm_min = now.time.minutes;
    rtc.tm_sec = now.time.seconds;
    return mktime(&rtc);
}

static time_t utcNow() { return M5.Rtc.isEnabled() ? rtcSeconds() : time(nullptr); }

static void utcSet(time_t seconds) {
    if (M5.Rtc.isEnabled()) {
        tm utc;
        gmtime_r(&seconds, &utc);
        M5.Rtc.setDateTime(&utc);
    } else {
        timeval tv = {seconds, 0};
        settimeofday(&tv, nullptr);
    }
}

void clockBegin() {
    if (utcNow() < (time_t)BUILD_UTC) utcSet(BUILD_UTC);
}

static volatile bool synced = false; // set by the network time service's thread, handled in clockSyncUpdate()
static bool syncing = false;

void clockSyncUpdate() {
    bool connected = wifiConnected();
    if (connected && !syncing) {
        sntp_set_time_sync_notification_cb([](timeval *) { synced = true; });
        configTime(0, 0, "pool.ntp.org", "time.google.com"); // UTC: the time zone is added when showing it
        syncing = true;
    } else if (!connected && syncing) {
        esp_sntp_stop();
        syncing = false;
    }

    if (synced) { // the system clock now has the network time; the RTC chip, if there is one, is what is read, so copy it there
        synced = false;
        if (M5.Rtc.isEnabled()) utcSet(time(nullptr));
    }
}

int clockLocalMinutes() {
    time_t local = utcNow() + timezoneMinutes() * 60;
    return local % 86400 / 60;
}

const char *clockText() {
    static char text[6];
    int minutes = clockLocalMinutes();
    snprintf(text, sizeof(text), "%02d:%02d", minutes / 60, minutes % 60);
    return text;
}

void clockSet(int hours, int minutes) {
    // Swap the local time of day and keep the local date; the UTC date may differ from it
    time_t offset = timezoneMinutes() * 60;
    time_t local = utcNow() + offset;
    local += (hours * 3600 + minutes * 60) - local % 86400;
    utcSet(local - offset);
}
