#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibIo Device Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <io/buffer.h>
#include <io/buffered_device.h>
#include <io/sstream_device.h>
#include <io/fragment.h>


using namespace uh;
using namespace uh::io;

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

typedef boost::mpl::vector<
    sstream_device,
    buffered_device<sstream_device>,
    buffer,
    fragment
> device_types;

// ---------------------------------------------------------------------

struct Fixture {};

// ---------------------------------------------------------------------

/**
 * To be implemented for each type in `device_types`: constructs a device
 * that will read the text given in TEST_TEXT.
 */
template <typename T>
std::unique_ptr<T> make_test_device();

// ---------------------------------------------------------------------

template <>
std::unique_ptr<sstream_device> make_test_device<sstream_device>()
{
    return std::make_unique<sstream_device>(TEST_TEXT);
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
    rv->write(TEST_TEXT);
    return rv;
}

// ---------------------------------------------------------------------

template <>
std::unique_ptr<fragment> make_test_device()
{
    static std::unique_ptr<buffer> base;
    base = make_test_device<buffer>();
    auto ser = serialization::serialization(*base);
    ser.write(TEST_TEXT);

    return std::make_unique<fragment>(*base);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( valid_default, T, device_types, Fixture )
{
    auto dev = make_test_device<T>();

    BOOST_CHECK(dev->valid());
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( read_full, T, device_types, Fixture )
{
    auto dev = make_test_device<T>();

    std::vector<char> buffer(TEST_TEXT.size());

    {
        auto count = dev->read(buffer);
        BOOST_CHECK_EQUAL(count, TEST_TEXT.size());
        BOOST_CHECK_EQUAL(TEST_TEXT, std::string(&buffer[0], count));
    }

    {
        auto count = dev->read(buffer);

        BOOST_CHECK_EQUAL(count, 0u);
    }

    BOOST_CHECK(!dev->valid());
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( read_partial, T, device_types, Fixture )
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

    BOOST_CHECK_EQUAL(total, TEST_TEXT.size());
    BOOST_CHECK_EQUAL(TEST_TEXT, complete);
    BOOST_CHECK(!dev->valid());
}

// ---------------------------------------------------------------------

} // namespace
