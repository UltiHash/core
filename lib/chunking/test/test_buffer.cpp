#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibChunking Buffer Tests"
#endif

#include <boost/test/unit_test.hpp>

#include <test/ipsum.h>
#include <io/sstream_device.h>
#include <chunking/buffer.h>


using namespace uh;
using namespace uh::chunking;
using namespace uh::test;

namespace
{

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( fill_buffer )
{
    io::sstream_device dev(LOREM_IPSUM);
    constexpr int size = 12;

    buffer b(dev, size);

    BOOST_CHECK_LE(size, b.fill_buffer());
    BOOST_CHECK_LE(size, b.length());

    for (unsigned i = 0; i < LOREM_IPSUM.size() / size; ++i)
    {
        b.skip(size);
        b.fill_buffer();
    }

    BOOST_CHECK_EQUAL(LOREM_IPSUM.size() % size, b.fill_buffer());
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( next_byte )
{
    io::sstream_device dev(LOREM_IPSUM);
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
    io::sstream_device dev(LOREM_IPSUM);
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
    io::sstream_device dev(LOREM_IPSUM);
    constexpr int size = 12;

    buffer b(dev, size);
    BOOST_CHECK_LE(size, b.fill_buffer());

    auto data = b.data();
    BOOST_CHECK_EQUAL("Lorem ipsum ", std::string(data.data(), size));
}

// ---------------------------------------------------------------------

} // namespace
