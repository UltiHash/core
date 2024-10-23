#include "formats.h"

#include <chrono>
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
    // time_point -> time_t ->  tm -> string
    std::stringstream ss;

    auto t = utc_time::clock::to_time_t(ts);
    tm buf;
    ss << std::put_time(gmtime_r(&t, &buf), "%FT%TZ");

    return ss.str();
}

using namespace std::chrono_literals;

namespace detail {

utc_time read_local_time(const std::string& str) {
    // sring -> tm -> time_t -> time_point
    std::istringstream ss(str);
    std::tm t = {};
    ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");
    return utc_time::clock::from_time_t(timegm(&t));
}

std::chrono::seconds read_timezone_offset(const std::string& str) {
    if (str.ends_with('Z')) {
        return 0s;
    } else if (str.size() < 6) {
        return 0s;
    } else {
        auto pol = *(str.end() - 6);
        if (pol != '+' && pol != '-') {
            return 0s;
        }
        std::istringstream ss(std::string(str.end() - 5, str.end()));
        tm t;
        // str.find_last_of("+-");
        ss >> std::get_time(&t, "%H:%M"); // Parse time in "HH:MM" format
        auto offset =
            std::chrono::hours(t.tm_hour) + std::chrono::minutes(t.tm_min);
        if (pol == '-')
            offset = -offset;
        return offset;
    }
}

} // namespace detail

utc_time read_iso8601_date(const std::string& str) {
    auto time = detail::read_local_time(str);
    auto offset = detail::read_timezone_offset(str);
    return time + offset;
}

} // namespace uh::cluster
