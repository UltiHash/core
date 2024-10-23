
#define BOOST_TEST_MODULE "Test formats"

#include "entrypoint/formats.h"
#include <boost/test/unit_test.hpp>

#include <iomanip>
#include <iostream>
#include <locale>
#include <random>
#include <sstream>

using namespace uh::cluster;
using namespace std::chrono_literals;

template <typename Clock, typename Duration>
std::ostream& operator<<(std::ostream& os,
                         const std::chrono::time_point<Clock, Duration>& tp) {
    std::time_t time = Clock::to_time_t(tp);
    return os << std::ctime(&time); // Convert time_point to readable string
}

BOOST_AUTO_TEST_CASE(read_local_time__handles_seconds) {
    auto str = std::string("2011-02-18T23:12:34Z");
    auto date = detail::read_local_time(str);
    auto outstr = iso8601_date(date);
    BOOST_CHECK_EQUAL(outstr, str);
}

BOOST_AUTO_TEST_CASE(read_local_time__drops_decimal_fraction_of_seconds) {
    auto date = detail::read_local_time(std::string("2011-02-18T23:12:34.5Z"));
    auto outstr = iso8601_date(date);
    BOOST_TEST(outstr == "2011-02-18T23:12:34Z");
}

BOOST_AUTO_TEST_CASE(read_local_time__dtops_info_after_false_format) {
    auto date = detail::read_local_time(std::string("2011-02-18X23:12:34Z"));
    auto outstr = iso8601_date(date);
    BOOST_TEST(outstr == "2011-02-18T00:00:00Z");
}

BOOST_AUTO_TEST_CASE(read_timezone_offset__handles_plus_offset) {
    auto str = std::string("+02:00");
    BOOST_TEST(detail::read_timezone_offset(str) == 2h);
}

BOOST_AUTO_TEST_CASE(read_timezone_offset__handles_minus_offset) {
    auto str = std::string("-02:00");
    BOOST_TEST(detail::read_timezone_offset(str) == -2h);
}

BOOST_AUTO_TEST_CASE(read_iso8601_date__reverses_iso8601_date) {
    auto now = std::chrono::time_point_cast<std::chrono::seconds>(
        std::chrono::system_clock::now());
    auto str = iso8601_date(now);
    BOOST_TEST(read_iso8601_date(str) == now);
}

BOOST_AUTO_TEST_CASE(iso8601_date__reverses_read_iso8601_date_when_TZD_is_Z) {
    auto str = std::string("2011-02-18T23:12:34Z");
    BOOST_TEST(iso8601_date(read_iso8601_date(str)) == str);
}

BOOST_AUTO_TEST_CASE(iso8601_date__prints_UTC_time) {
    auto str = std::string("2011-02-18T23:12:34-02:00");
    BOOST_TEST(iso8601_date(read_iso8601_date(str)) == "2011-02-18T21:12:34Z");
}

std::chrono::system_clock::time_point generate_random_timepoint() {
    std::random_device rd;
    std::mt19937 gen(rd());

    // Epoch range (1970-2030)
    std::uniform_int_distribution<std::time_t> dist(0, 1893456000);

    return std::chrono::system_clock::from_time_t(dist(gen));
}

BOOST_AUTO_TEST_CASE(read_iso8601_date_survive_on_random_test) {
    auto now = generate_random_timepoint();
    auto str = iso8601_date(now);
    BOOST_TEST(read_iso8601_date(str) == now);
}
