#include <Tachyon/Time.hpp>
#include <Common.hpp>
#include <chrono>
#include <ctime>

using namespace Tachyon;

double __hot Time::GetSeconds(void) {
    const time_t UnixTimestamp = time(nullptr);
    const struct tm CurrentTime = *localtime(&UnixTimestamp);
    return CurrentTime.tm_sec;
}

double __hot Time::GetMinute(void) {
    const time_t UnixTimestamp = time(nullptr);
    const struct tm CurrentTime = *localtime(&UnixTimestamp);
    return CurrentTime.tm_min;
}

double __hot Time::GetHour(void) {
    const time_t UnixTimestamp = time(nullptr);
    const struct tm CurrentTime = *localtime(&UnixTimestamp);
    return CurrentTime.tm_hour;
}

double __hot Time::GetDayOfWeek(void) {
    const time_t UnixTimestamp = time(nullptr);
    const struct tm CurrentTime = *localtime(&UnixTimestamp);
    return (CurrentTime.tm_wday + 1);
}

double __hot Time::GetDayOfMonth(void) {
    const time_t UnixTimestamp = time(nullptr);
    const struct tm CurrentTime = *localtime(&UnixTimestamp);
    return CurrentTime.tm_mday;
}

double __hot Time::GetMonth(void) {
    const time_t UnixTimestamp = time(nullptr);
    const struct tm CurrentTime = *localtime(&UnixTimestamp);
    return (CurrentTime.tm_mon + 1);
}


double __hot Time::GetYear(void) {
    const time_t UnixTimestamp = time(nullptr);
    const struct tm CurrentTime = *localtime(&UnixTimestamp);
    return (CurrentTime.tm_year + 1900);
}

double __hot Time::GetDaysSince2000(void) {
    /* boy is lambda useful */
    static const auto start = []() {
        struct tm start_tm = {.tm_sec = 0, .tm_min = 0, .tm_hour = 0, .tm_mday = 1, .tm_mon = 0, .tm_year = 2000 - 1900};
        return std::chrono::system_clock::from_time_t(mktime(&start_tm));
    }();

    const auto now = std::chrono::system_clock::now();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

    return (millis / 86400000.0);
}