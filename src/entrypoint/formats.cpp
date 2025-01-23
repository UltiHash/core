#include "formats.h"

#include <common/utils/strings.h>

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
constexpr auto CONSTEXPR_SIZE = std::char_traits<char>::length;
constexpr auto DATE_LEN = CONSTEXPR_SIZE("2011-02-18T23:12:34");
constexpr auto TZ_LEN = CONSTEXPR_SIZE("+02:00");
constexpr auto MAX_YEAR = 2261;
constexpr auto TM_YEAR_OFFSET = 1900;

inline std::runtime_error create_time_format_error() {
    return std::runtime_error(R"(
time format error:
    - `2011-02-18T23:12:34-02:00` and `2011-02-18T23:12:34Z` formats are supported
    - Constaints should be less than `2270-01-01T00:00:00Z`)");
}

utc_time read_iso8601_date(std::string_view str) {

    if (str.size() < DATE_LEN)
        throw create_time_format_error();

    auto date_str = std::string_view(str).substr(0, DATE_LEN);
    auto tz_str = std::string_view(str).substr(DATE_LEN, str.size() - DATE_LEN);

    auto time = detail::read_local_date(date_str);
    auto offset = detail::read_timezone(tz_str);
    return time + offset;
}

utc_time read_iso8601_date_merged(std::string_view s) {
    if (s.size() != 16 || s[8] != 'T' || s[15] != 'Z') {
        throw std::runtime_error("malformed date");
    }

    std::size_t pos = 0;

    int year = stoul(s.substr(0, 4), &pos);
    if (pos != 4) {
        throw std::runtime_error("malformed date");
    }

    int month = stoul(s.substr(4, 2), &pos);
    if (pos != 2) {
        throw std::runtime_error("malformed date");
    }

    int day = stoul(s.substr(6, 2), &pos);
    if (pos != 2) {
        throw std::runtime_error("malformed date");
    }

    int hour = stoul(s.substr(9, 2), &pos);
    if (pos != 2) {
        throw std::runtime_error("malformed date");
    }

    int minute = stoul(s.substr(11, 2), &pos);
    if (pos != 2) {
        throw std::runtime_error("malformed date");
    }

    int second = stoul(s.substr(13, 2), &pos);
    if (pos != 2) {
        throw std::runtime_error("malformed date");
    }

    return make_utc_time(year, month, day, hour, minute, second);
}

utc_time make_utc_time(int year, int month, int day, int hour, int min,
                       int sec) {
    std::tm t{};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;

    t.tm_hour = hour;
    t.tm_min = min;
    t.tm_sec = sec;

    return utc_time::clock::from_time_t(timegm(&t));
}

namespace detail {

// Flow: string -> tm -> time_t -> time_point
utc_time read_local_date(std::string_view sv) {
    if (sv.size() != DATE_LEN) [[unlikely]]
        throw create_time_format_error();

    std::istringstream ss;
    ss.rdbuf()->pubsetbuf(const_cast<char*>(sv.data()), sv.size());

    std::tm t = {};
    ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");

    if (ss.fail()) [[unlikely]]
        throw create_time_format_error();

    if (t.tm_year + TM_YEAR_OFFSET >= MAX_YEAR) [[unlikely]]
        throw create_time_format_error();

    return utc_time::clock::from_time_t(timegm(&t));
}

std::chrono::hours read_timezone(std::string_view sv) {
    if (sv == "Z") [[unlikely]]
        return 0h;

    if (sv.size() != TZ_LEN) [[unlikely]]
        throw create_time_format_error();

    auto& pol = sv[0];
    if (pol != '+' && pol != '-') [[unlikely]]
        throw create_time_format_error();

    // Drop sign
    std::istringstream ss;
    ss.rdbuf()->pubsetbuf(const_cast<char*>(sv.data() + 1), sv.size() - 1);

    tm t;
    ss >> std::get_time(&t, "%H:00"); // Parse time in "HH:00" format
    if (ss.fail()) [[unlikely]]
        throw create_time_format_error();

    if (t.tm_hour <= -24 || t.tm_hour >= 24) [[unlikely]]
        throw create_time_format_error();

    auto offset = std::chrono::hours(t.tm_hour);
    if (pol == '-')
        offset = -offset;
    return offset;
}

} // namespace detail

} // namespace uh::cluster

namespace std {

ostream& operator<<(ostream& out, const uh::cluster::utc_time& t) {
    out << uh::cluster::iso8601_date(t);
    return out;
}

} // namespace std
