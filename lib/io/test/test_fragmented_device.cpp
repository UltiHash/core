//
// Created by benjamin-elias on 17.05.23.
//


#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibUtil fragmentedDevice Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <util/exception.h>
#include <io/fragment.h>
#include <io/buffer.h>

using namespace uh::util;
using namespace uh::io;

namespace
{

// ---------------------------------------------------------------------

const std::string LOREM_IPSUM =
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

struct Fixture {};

// ---------------------------------------------------------------------

typedef boost::mpl::vector<
        fragment
> device_types;

// ---------------------------------------------------------------------

/**
 * To be implemented for each type in `device_types`: constructs a device
 * that will read the text given in TEST_TEXT.
 */
template <typename T>
std::unique_ptr<T> make_test_device();

// ---------------------------------------------------------------------

template <>
std::unique_ptr<fragment> make_test_device<fragment>()
{
    static std::unique_ptr<buffer> buf;
    buf = std::make_unique<buffer>();

    auto rv = std::make_unique<fragment>(*buf);

    return rv;
}


BOOST_FIXTURE_TEST_CASE_TEMPLATE( multi_fragment_on_device_test, T, device_types, Fixture )
{
    std::unique_ptr<T> fragmented = make_test_device<T>();
    fragmented->write(LOREM_IPSUM);
    fragmented->write(LOREM_IPSUM+"another ipsum");

    std::vector<char> read_back_first{};
    fragmented->read({read_back_first.data(),read_back_first.size()});
}

// ---------------------------------------------------------------------

} // namespace