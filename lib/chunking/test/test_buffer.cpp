#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibChunking Buffer Tests"
#endif

#include <boost/test/unit_test.hpp>

#include <io/sstream_device.h>
#include <chunking/buffer.h>


using namespace uh;
using namespace uh::chunking;

namespace
{

// ---------------------------------------------------------------------

const static std::string TEST_TEXT =
    "Lorem ipsum dolor sit amet, consectetur adipiscing "
    "elit, sed do eiusmod tempor incididunt ut labore et "
    "dolore magna aliqua. Ut enim ad minim veniam, quis "
    "nostrud exercitation ullamco laboris nisi ut "
    "aliquip ex ea commodo consequat. Duis aute irure "
    "dolor in reprehenderit in voluptate velit esse "
    "cillum dolore eu fugiat nulla pariatur. Excepteur "
    "sint occaecat cupidatat non proident, sunt in culpa "
    "qui officia deserunt mollit anim id est laborum.";

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( fill_buffer )
{
    io::sstream_device dev(TEST_TEXT);
    constexpr int size = 12;

    buffer b(dev, size);

    BOOST_CHECK_LE(size, b.fill_buffer());
    BOOST_CHECK_LE(size, b.length());

    for (unsigned i = 0; i < TEST_TEXT.size() / size; ++i)
    {
        b.skip(size);
        b.fill_buffer();
    }

    BOOST_CHECK_EQUAL(TEST_TEXT.size() % size, b.fill_buffer());
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( next_byte )
{
    io::sstream_device dev(TEST_TEXT);
    constexpr int size = 12;

    buffer b(dev, size);

    auto count = b.fill_buffer();
    BOOST_CHECK_LE(size, b.fill_buffer());

    for (unsigned i = 0; i < count; ++i)
    {
        b.next_byte();
    }

    BOOST_CHECK_EQUAL(-1, b.next_byte());
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( mark_data )
{
    io::sstream_device dev(TEST_TEXT);
    constexpr int size = 12;

    buffer b(dev, size);
    BOOST_CHECK_LE(size, b.fill_buffer());

    b.skip(6);
    auto pos = b.mark();
    b.skip(5);

    auto data = b.data(pos);
    BOOST_CHECK_EQUAL("ipsum", std::string(data.data(), data.size()));
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( data )
{
    io::sstream_device dev(TEST_TEXT);
    constexpr int size = 12;

    buffer b(dev, size);
    BOOST_CHECK_LE(size, b.fill_buffer());

    auto data = b.data();
    BOOST_CHECK_EQUAL("Lorem ipsum ", std::string(data.data(), size));
}

// ---------------------------------------------------------------------

} // namespace
