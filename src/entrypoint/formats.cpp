#include "formats.h"

#include <chrono>
#include <ctime>
#include <stdexcept>

namespace uh::cluster {

std::string imf_fixdate(const utc_time& ts) {
    std::stringstream ss;

    auto t = utc_time::clock::to_time_t(ts);
    tm buf;
    ss << std::put_time(gmtime_r(&t, &buf), "%a, %d %b %Y %H:%M:%S %Z");

    return ss.str();
}

// Flow: time_point -> time_t ->  tm -> string
std::string iso8601_date(const utc_time& ts) {
    std::stringstream ss;

    auto t = utc_time::clock::to_time_t(ts);
    tm buf;
    ss << std::put_time(gmtime_r(&t, &buf), "%FT%TZ");

    return ss.str();
}

using namespace std::chrono_literals;

/*
 * Constants, related to specification
 */
constexpr auto constexpr_size = std::char_traits<char>::length;
constexpr auto date_len = constexpr_size("2011-02-18T23:12:34");
constexpr auto tz_len = constexpr_size("+02:00");

inline std::runtime_error create_time_format_error() {
    return std::runtime_error("Time format error: `2011-02-18T23:12:34-02:00` "
                              "and `2011-02-18T23:12:34Z` supported");
}

utc_time read_iso8601_date(std::string_view str) {

    auto date_str = std::string_view(str).substr(0, date_len);
    auto tz_str = std::string_view(str).substr(date_len, str.size() - date_len);

    auto time = detail::read_local_date(date_str);
    auto offset = detail::read_timezone(tz_str);
    return time + offset;
}

namespace detail {

// Flow: string -> tm -> time_t -> time_point
utc_time read_local_date(std::string_view sv) {
    if (sv.size() != date_len)
        throw create_time_format_error();

    std::istringstream ss;
    ss.rdbuf()->pubsetbuf(const_cast<char*>(sv.data()), sv.size());

    std::tm t = {};
    ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail())
        throw create_time_format_error();

    return utc_time::clock::from_time_t(timegm(&t));
}

std::chrono::seconds read_timezone(std::string_view sv) {
    if (sv == "Z")
        return 0s;

    if (sv.size() != tz_len)
        throw create_time_format_error();

    auto& pol = sv[0];
    if (pol != '+' && pol != '-')
        throw create_time_format_error();

    // Drop sign
    std::istringstream ss;
    ss.rdbuf()->pubsetbuf(const_cast<char*>(sv.data() + 1), sv.size() - 1);

    tm t;
    ss >> std::get_time(&t, "%H:00"); // Parse time in "HH:00" format
    if (ss.fail())
        throw create_time_format_error();

    if (t.tm_hour <= -24 || t.tm_hour >= 24)
        throw create_time_format_error();

    auto offset = std::chrono::hours(t.tm_hour);
    if (pol == '-')
        offset = -offset;
    return offset;
}

} // namespace detail

} // namespace uh::cluster
