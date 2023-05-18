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

#include <random>

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

struct Fixture {};

// ---------------------------------------------------------------------

std::string gen_random() {
    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    std::string tmp_s;
    tmp_s.reserve(6);
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 rand_gen(seed);
    std::uniform_int_distribution<uint16_t> distribution(0,sizeof(alphanum));

    for (int i = 0; i < 6; ++i) {
        tmp_s += alphanum[distribution(rand_gen) % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( seek_unspecified, T, device_types, Fixture )
{
    auto test_path = std::filesystem::path(TEMP_DIR);

    if constexpr (std::is_same_v<T,file>){
        auto old_path = test_path;

        do{
            test_path = old_path / ("tempfile-" + gen_random());
        } while (std::filesystem::exists(test_path));
    }

    {
        T tf(test_path, std::ios_base::out);
        test_path = tf.path();

        auto written = tf.write(LOREM_IPSUM);
        BOOST_CHECK_EQUAL(written, LOREM_IPSUM.size());

        BOOST_REQUIRE(tf.valid());

        if constexpr (std::is_same_v<T, temp_file>)
        {
            tf.release_to(test_path);
        }
    }

    file tf2(test_path, std::ios_base::in);
    tf2.seek(10, std::ios_base::cur);

    std::string copy(LOREM_IPSUM.size()-10, 0);

    auto read = tf2.read(copy);
    BOOST_CHECK_EQUAL(read, LOREM_IPSUM.size()-10);

    auto test_string = std::string{LOREM_IPSUM.begin()+10,LOREM_IPSUM.end()};
    BOOST_CHECK_EQUAL(copy, test_string);

    BOOST_REQUIRE(tf2.valid());

    std::filesystem::remove(test_path);
}

// ---------------------------------------------------------------------

} // namespace
