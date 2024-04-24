#include "formats.h"

#include <ctime>
#include <iomanip>

namespace uh::cluster {

std::string imf_fixdate(const utc_time& ts) {
    std::stringstream ss;

    auto t = utc_time::clock::to_time_t(ts);
    tm buf;
    ss << std::put_time(gmtime_r(&t, &buf), "%a, %d %b %Y %H:%M:%S %Z");

    return ss.str();
}

std::string iso8601_date(const utc_time& ts) {
    std::stringstream ss;

    auto t = utc_time::clock::to_time_t(ts);
    tm buf;
    ss << std::put_time(gmtime_r(&t, &buf), "%FT%TZ");

    return ss.str();
}

} // namespace uh::cluster
