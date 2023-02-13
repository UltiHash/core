#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibProtocol Message Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <protocol/exception.h>
#include <protocol/serializer.h>

#include <sstream>


using namespace uh::protocol;

namespace
{

// ---------------------------------------------------------------------

template <typename T>
T copy(T value)
{
    std::stringstream s;

    write(s, value);

    T rv;
    read(s, rv);

    return rv;
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( integral_types )
{
    BOOST_TEST(copy(static_cast<uint8_t>(0x55)) == 0x55);
    BOOST_TEST(copy(static_cast<uint32_t>(0xdeadbeef)) == 0xdeadbeef);
    BOOST_TEST(copy(static_cast<int32_t>(0xdeadbeef)) == 0xdeadbeef);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( vector )
{
    std::vector<char> orig{ 100, 123, 0x55, 0x23 };
    BOOST_TEST(copy(orig) == orig);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( string )
{
    std::string orig = "Lorem ipsum dolor sit amet, consectetur adipiscing elit";
    BOOST_TEST(copy(orig) == orig);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( error_too_big )
{
    {
        std::stringstream s;

        std::vector<char> value;
        value.resize(MAX_BLOB_LENGTH + 1);

        BOOST_CHECK_THROW(write(s, value), write_limit_exceeded);
    }

    {
        std::stringstream s;

        std::string value;
        value.resize(MAX_STRING_LENGTH + 1);

        BOOST_CHECK_THROW(write(s, value), write_limit_exceeded);
    }
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( error_read_past_end )
{
    {
        std::stringstream s;
        std::vector<char> value;
        BOOST_CHECK_THROW(read(s, value), read_error);
    }

    {
        std::stringstream s;
        std::string value;
        BOOST_CHECK_THROW(read(s, value), read_error);
    }

    {
        std::stringstream s;
        uint8_t value;
        BOOST_CHECK_THROW(read(s, value), read_error);
    }

    {
        std::stringstream s;
        uint32_t value;
        BOOST_CHECK_THROW(read(s, value), read_error);
    }

    {
        std::stringstream s;
        int32_t value;
        BOOST_CHECK_THROW(read(s, value), read_error);
    }
}

// ---------------------------------------------------------------------

}
