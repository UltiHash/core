//
// Created by benjamin-elias on 12.05.23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibUtil seekableDevice Tests"
#endif

#include <test/ipsum.h>
#include <util/random.h>
#include <io/file.h>
#include <io/temp_file.h>

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>


using namespace uh::io;
using namespace uh::util;
using namespace uh::test;

namespace
{

// ---------------------------------------------------------------------

const std::filesystem::path TEMP_DIR = "/tmp";

// ---------------------------------------------------------------------

typedef boost::mpl::vector<
    file,
    temp_file
> device_types;

// ---------------------------------------------------------------------

struct Fixture {};

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE(seek_unspecified, T, device_types, Fixture )
{
    auto test_path = std::filesystem::path(TEMP_DIR);

    if constexpr (std::is_same_v<T,file>){
        auto old_path = test_path;

        do{
            test_path = old_path / ("tempfile-" + random_string(6));
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
    tf2.close();
    tf2.reopen();
    tf2.seek(10, std::ios_base::cur);

    std::string copy(LOREM_IPSUM.size()-10, 0);

    auto read = tf2.read(copy);
    BOOST_CHECK_EQUAL(read, LOREM_IPSUM.size() - 10);

    auto test_string = std::string{LOREM_IPSUM.begin() + 10, LOREM_IPSUM.end()};
    BOOST_CHECK_EQUAL(copy, test_string);

    BOOST_REQUIRE(tf2.valid());

    std::filesystem::remove(test_path);
}

// ---------------------------------------------------------------------

} // namespace
