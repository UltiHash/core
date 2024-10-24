
#include <boost/throw_exception.hpp>
#include <stdexcept>
#define BOOST_TEST_MODULE "formats"

#include "entrypoint/formats.h"
#include <boost/test/data/monomorphic.hpp> // for data driven tests
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>

using namespace uh::cluster;
using namespace std::chrono_literals;
namespace bdata = boost::unit_test::data;

template <typename Clock, typename Duration>
std::ostream& operator<<(std::ostream& os,
                         const std::chrono::time_point<Clock, Duration>& tp) {
    std::time_t time = Clock::to_time_t(tp);
    return os << std::ctime(&time); // Convert time_point to readable string
}

/******************************************************************************
 * Tests for read_iso8601
 */

BOOST_AUTO_TEST_CASE(read_iso8601_date__reverses_iso8601_date) {
    auto now = std::chrono::system_clock::now();
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

BOOST_DATA_TEST_CASE(read_iso8601_date_survive_on_random_test,
                     bdata::random(0, 1893456000) /*1970-2030*/ ^
                         bdata::xrange(100),
                     time, index) {
    auto tp = std::chrono::system_clock::from_time_t(time);
    auto str = iso8601_date(tp);
    BOOST_TEST(read_iso8601_date(str) == tp);
}

/******************************************************************************
 * Tests for functions in detail namespace.
 */

BOOST_AUTO_TEST_CASE(read_local_date__handles_seconds) {
    // Arrange
    auto str = "2011-02-18T23:12:34";
    // Act
    auto date = detail::read_local_date(str);
    // Assert
    BOOST_CHECK_EQUAL(iso8601_date(date), "2011-02-18T23:12:34Z");
}

BOOST_AUTO_TEST_CASE(read_local_date__does_not_handle_decimal_fraction) {
    // Arrange
    auto str = "2011-02-18T23:12:34.5";
    // Act and Assert
    BOOST_REQUIRE_THROW(detail::read_local_date(str), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(read_local_date__does_not_handle_wrong_input) {
    auto str = "2011-02-18X23:12:34";

    BOOST_REQUIRE_THROW(detail::read_local_date(str), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(read_timezone__handles_Z) {
    auto str = "Z";

    auto offset = detail::read_timezone(str);

    BOOST_TEST(offset == 0h);
}

BOOST_AUTO_TEST_CASE(read_timezone__handles_plus_TZ) {
    auto str = "+02:00";

    auto offset = detail::read_timezone(str);

    BOOST_TEST(offset == 2h);
}

BOOST_AUTO_TEST_CASE(read_timezone__handles_minus_TZ) {
    auto str = "-02:00";

    auto offset = detail::read_timezone(str);

    BOOST_TEST(offset == -2h);
}

BOOST_DATA_TEST_CASE(read_timezone_handles_offsets_in_reasonable_range,
                     bdata::xrange(-23, 24)) {
    std::stringstream ss;
    ss << ((sample < 0) ? "-" : "+");
    ss << std::setw(2) << std::setfill('0') << std::abs(sample) << ":00";

    auto offset = detail::read_timezone(ss.str());

    BOOST_TEST(offset == std::chrono::hours(sample));
}

BOOST_DATA_TEST_CASE(read_timezone__does_not_handle_wrong_input,
                     bdata::make(std::vector<std::string>{
                         "-24:00", "+24:00", "+00:01", "X", "0Z", "00:00",
                         "+0:00", "+00:0", "*00:00Z", "+00:00Z"}),
                     str) {
    BOOST_REQUIRE_THROW(detail::read_timezone(str), std::runtime_error);
}
