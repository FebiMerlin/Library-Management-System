#include "lms/date.h"

#include <chrono>
#include <cstdio>

namespace lms {

// Calendar conversions use the algorithms from Howard Hinnant's
// "chrono-Compatible Low-Level Date Algorithms" (public domain).

Day currentDay() {
    using namespace std::chrono;
    const auto now = system_clock::now().time_since_epoch();
    return static_cast<Day>(duration_cast<hours>(now).count() / 24);
}

std::string formatDay(Day day) {
    long long z = day;
    z += 719468;
    const long long era = (z >= 0 ? z : z - 146096) / 146097;
    const long long doe = z - era * 146097;
    const long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long long y = yoe + era * 400;
    const long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const long long mp = (5 * doy + 2) / 153;
    const long long d = doy - (153 * mp + 2) / 5 + 1;
    const long long m = mp < 10 ? mp + 3 : mp - 9;
    const long long year = y + (m <= 2 ? 1 : 0);

    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04lld-%02lld-%02lld", year, m, d);
    return buf;
}

bool parseDay(const std::string& text, Day& out) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') return false;
    int y = 0, m = 0, d = 0;
    for (int i : {0, 1, 2, 3, 5, 6, 8, 9}) {
        if (text[static_cast<std::size_t>(i)] < '0' || text[static_cast<std::size_t>(i)] > '9') return false;
    }
    y = (text[0] - '0') * 1000 + (text[1] - '0') * 100 + (text[2] - '0') * 10 + (text[3] - '0');
    m = (text[5] - '0') * 10 + (text[6] - '0');
    d = (text[8] - '0') * 10 + (text[9] - '0');
    if (m < 1 || m > 12 || d < 1 || d > 31) return false;

    const long long yy = y - (m <= 2 ? 1 : 0);
    const long long era = (yy >= 0 ? yy : yy - 399) / 400;
    const long long yoe = yy - era * 400;
    const long long mp = m > 2 ? m - 3 : m + 9;
    const long long doy = (153 * mp + 2) / 5 + d - 1;
    const long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    out = static_cast<Day>(era * 146097 + doe - 719468);
    return formatDay(out) == text;  // rejects e.g. 2026-02-31
}

}  // namespace lms
