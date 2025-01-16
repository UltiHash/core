
#include <boost/throw_exception.hpp>
#include <stdexcept>
#define BOOST_TEST_MODULE "formats"

#include "entrypoint/formats.h"
#include <boost/test/data/monomorphic.hpp> // for data driven tests
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include <iomanip>
#include <sstream>

using namespace uh::cluster;
using namespace std::chrono_literals;
namespace bdata = boost::unit_test::data;

/******************************************************************************
 * Tests for read_iso8601
 */

BOOST_AUTO_TEST_CASE(iso8601_date__reverses_read_iso8601_date_when_TZD_is_Z) {
    // Arrange
    auto str = std::string("2011-02-18T23:12:34Z");
    // Act
    auto read_date = iso8601_date(read_iso8601_date(str));
    // Assert
    BOOST_TEST(read_date == str);
}

BOOST_AUTO_TEST_CASE(iso8601_date__handles_minus_timeoffset) {
    auto str = std::string("2011-02-18T23:12:34-02:00");

    auto read_date = iso8601_date(read_iso8601_date(str));

    BOOST_TEST(read_date == "2011-02-18T21:12:34Z");
}

BOOST_AUTO_TEST_CASE(iso8601_date__handles_plus_timeoffset) {
    auto str = std::string("2011-02-18T23:12:34+02:00");

    auto read_date = iso8601_date(read_iso8601_date(str));

    BOOST_TEST(read_date == "2011-02-19T01:12:34Z");
}

BOOST_DATA_TEST_CASE(iso8601_date__handles_offsets_in_reasonable_range,
                     bdata::xrange(24)) {
    constexpr auto hour_offset = std::char_traits<char>::length("2020-01-01T");
    auto base_date = std::string("2020-01-01T00:00:00");

    std::stringstream ss;
    auto utc_date = base_date + "Z";
    ss << std::setw(2) << std::setfill('0') << sample;
    utc_date[hour_offset] = ss.str()[0];
    utc_date[hour_offset + 1] = ss.str()[1];

    ss.clear();
    ss.str("");
    ss << base_date << "+" << std::setw(2) << std::setfill('0') << sample
       << ":00";
    auto date_with_timezone = iso8601_date(read_iso8601_date(ss.str()));

    BOOST_TEST(utc_date == date_with_timezone);
}

BOOST_DATA_TEST_CASE(read_timezone__does_not_handle_wrong_input,
                     bdata::make(std::vector<std::string>{
                         "", "-24:00", "+24:00", "+00:01", "X", "0Z", "00:00",
                         "+0:00", "+00:0", "*00:00Z", "+00:00Z"}),
                     str) {
    BOOST_REQUIRE_THROW(read_iso8601_date("2020-01-01T" + str),
                        std::runtime_error);
}

BOOST_AUTO_TEST_CASE(read_iso8601_date__does_not_handle_decimal_fraction) {
    // Arrange
    auto str = "2011-02-18T23:12:34.5Z";
    // Act and Assert
    BOOST_REQUIRE_THROW(read_iso8601_date(str), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(read_local_date__does_not_handle_wrong_input) {
    auto str = "2011-02-18X23:12:34Z";

    BOOST_REQUIRE_THROW(read_iso8601_date(str), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(read_iso8601_date__handles_the_year_2260) {
    auto str = std::string("2260-01-01T00:00:00Z");

    auto read_date = iso8601_date(read_iso8601_date(str));

    BOOST_TEST(read_date == str);
}

BOOST_AUTO_TEST_CASE(read_iso8601_date__doesnt_handle_the_year_2261) {
    auto str = std::string("2261-01-01T00:00:00Z");

    BOOST_REQUIRE_THROW(read_iso8601_date(str), std::runtime_error);
}

BOOST_DATA_TEST_CASE(read_iso8601_date_survive_on_random_test,
                     bdata::random(0, 1893456000) /*1970-2030*/ ^
                         bdata::xrange(100),
                     seconds, index) {
    auto time = iso8601_date(std::chrono::system_clock::from_time_t(seconds));

    auto read_time = iso8601_date(read_iso8601_date(time));

    BOOST_TEST(read_time == time);
}
