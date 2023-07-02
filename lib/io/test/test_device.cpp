#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibIo Device Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <test/ipsum.h>
#include <io/buffer.h>
#include <io/buffered_device.h>
#include <io/sstream_device.h>
#include <io/test_generator.h>


using namespace uh;
using namespace uh::io;
using namespace uh::test;

namespace
{

// ---------------------------------------------------------------------

typedef boost::mpl::vector<
    sstream_device,
    buffered_device<sstream_device>,
    buffer
> device_types;

// ---------------------------------------------------------------------

struct Fixture {};

// ---------------------------------------------------------------------

/**
 * To be implemented for each type in `device_types`: constructs a device
 * that will read the text given in LOREM_IPSUM.
 */
template <typename T>
std::unique_ptr<T> make_test_device();

// ---------------------------------------------------------------------

template <>
std::unique_ptr<sstream_device> make_test_device<sstream_device>()
{
    return std::make_unique<sstream_device>(LOREM_IPSUM);
}

// ---------------------------------------------------------------------

template <>
std::unique_ptr<buffered_device<sstream_device>> make_test_device<buffered_device<sstream_device>>()
{
    static std::unique_ptr<sstream_device> base;
    base = make_test_device<sstream_device>();

    return std::make_unique<buffered_device<sstream_device>>(*base);
}

// ---------------------------------------------------------------------

template <>
std::unique_ptr<buffer> make_test_device()
{
    auto rv = std::make_unique<buffer>();
    rv->write(LOREM_IPSUM);
    return rv;
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE(valid_default, T, device_types, Fixture )
{
    auto dev = make_test_device<T>();

    BOOST_CHECK(dev->valid());
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE(read_full, T, device_types, Fixture )
{
    auto dev = make_test_device<T>();

    std::vector<char> buffer(LOREM_IPSUM.size());

    {
        auto count = dev->read(buffer);
        BOOST_CHECK_EQUAL(count, LOREM_IPSUM.size());
        BOOST_CHECK_EQUAL(LOREM_IPSUM, std::string(&buffer[0], count));
    }

    {
        auto count = dev->read(buffer);

        BOOST_CHECK_EQUAL(count, 0u);
    }

    BOOST_CHECK(!dev->valid());
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE(read_partial, T, device_types, Fixture )
{
    auto dev = make_test_device<T>();

    std::vector<char> buffer(8);
    std::string complete;
    std::size_t total = 0;

    while (dev->valid())
    {
        auto count = dev->read(buffer);
        total += count;
        complete += std::string(&buffer[0], count);
    }

    BOOST_CHECK_EQUAL(total, LOREM_IPSUM.size());
    BOOST_CHECK_EQUAL(LOREM_IPSUM, complete);
    BOOST_CHECK(!dev->valid());
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE(data_generator_api, T, device_types, Fixture )
{
    std::vector v(LOREM_IPSUM.begin(), LOREM_IPSUM.end());
    io::test::generator gen(v);

    auto dev = make_test_device<T>();

    auto count = dev->write_range(gen);
    BOOST_CHECK_EQUAL(count, LOREM_IPSUM.size());
}

// ---------------------------------------------------------------------

} // namespace
