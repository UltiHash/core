#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibChunking Buffer Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <io/sstream_device.h>


using namespace uh;

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

typedef boost::mpl::vector<io::sstream_device> device_types;

// ---------------------------------------------------------------------

/**
 * To be implemented for each type in `device_types`: constructs a device
 * that will read the text given in TEST_TEXT.
 */
template <typename T>
std::unique_ptr<io::device> make_test_device();

// ---------------------------------------------------------------------

template <>
std::unique_ptr<io::device> make_test_device<io::sstream_device>()
{
    return std::make_unique<io::sstream_device>(TEST_TEXT);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( valid_default, T, device_types, T )
{
    auto dev = make_test_device<T>();

    BOOST_CHECK(dev->valid());
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( read_full, T, device_types, T )
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

BOOST_FIXTURE_TEST_CASE_TEMPLATE( read_partial, T, device_types, T )
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
