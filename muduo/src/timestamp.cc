#include "timestamp.h"

#include <sys/time.h>

using namespace muduo;

std::string Timestamp::ToString() const {
    char buf[32] = {0};
    uint64_t seconds = _ms_since_epoch / kMicroSecondsPerSecond;
    uint64_t ms = _ms_since_epoch % kMicroSecondsPerSecond;
    snprintf(buf, sizeof(buf), "%lu.%06lu", seconds, ms);
    return buf;
}

std::string Timestamp::ToFormattedString(bool show_ms) const {
    char buf[64] = {0};
    time_t seconds = static_cast<time_t>(_ms_since_epoch/kMicroSecondsPerSecond);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);
    if (show_ms) {
        int ms = static_cast<int>(_ms_since_epoch % kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), 
                "%4d%02d%02d %02d:%02d:%02d.%06d",
                tm_time.tm_year + 1900,
                tm_time.tm_mon + 1,
                tm_time.tm_mday,
                tm_time.tm_hour,
                tm_time.tm_min,
                tm_time.tm_sec,
                ms);
    } else {
        snprintf(buf, sizeof(buf), 
                "%4d%02d%02d %02d:%02d:%02d",
                tm_time.tm_year + 1900,
                tm_time.tm_mon + 1,
                tm_time.tm_mday,
                tm_time.tm_hour,
                tm_time.tm_min,
                tm_time.tm_sec);
    }

    return buf;
}

Timestamp Timestamp::Now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}
