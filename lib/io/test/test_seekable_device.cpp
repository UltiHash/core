//
// Created by benjamin-elias on 12.05.23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibUtil seekableDevice Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <util/exception.h>
#include <io/file.h>
#include <io/temp_file.h>

using namespace uh::util;
using namespace uh::io;

namespace
{

// ---------------------------------------------------------------------

    const std::filesystem::path TEMP_DIR = "/tmp";

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

    typedef boost::mpl::vector<
            file,
            temp_file
    > device_types;

// ---------------------------------------------------------------------

    /**
    * To be implemented for each type in `device_types`: constructs a device
    * that will read the text given in TEST_TEXT.
    */
    template <typename T>
    std::unique_ptr<T> make_test_seekable_device();

// ---------------------------------------------------------------------

    template <>
    std::unique_ptr<file> make_test_seekable_device<file>()
    {
        return std::make_unique<file>("/path");
    }

// ---------------------------------------------------------------------

    template <>
    std::unique_ptr<temp_file> make_test_seekable_device<temp_file>()
    {
        return std::make_unique<temp_file>("/path");
    }

// ---------------------------------------------------------------------

struct Fixture {};

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( seek_unspecified, T, device_types, Fixture )
{
    auto dev = make_test_seekable_device<T>();

    auto test_path = std::filesystem::path(TEMP_DIR);

    if constexpr (std::is_same_v<T,file>){
        test_path = temp_file::generate_valid_temp_path(TEMP_DIR);
    }

    T tf(test_path);

    auto written = tf.write({LOREM_IPSUM.c_str(), LOREM_IPSUM.size()});
    BOOST_CHECK_EQUAL(written, LOREM_IPSUM.size());

    file in(tf.path());

    in.seek(10);

    std::string copy(LOREM_IPSUM.size()-10, 0);
    auto read = in.read({copy.data(), copy.size()});
    BOOST_CHECK_EQUAL(read, LOREM_IPSUM.size()-10);

    auto test_string = std::string{LOREM_IPSUM.begin()+10,LOREM_IPSUM.end()};

    BOOST_CHECK_EQUAL(copy, test_string);

    BOOST_CHECK(dev->valid());
}

// ---------------------------------------------------------------------

} // namespace