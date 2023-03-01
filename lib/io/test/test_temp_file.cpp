#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibUtil TempFile Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <util/exception.h>
#include <io/temp_file.h>


using namespace uh::util;
using namespace uh::io;

namespace
{

// ---------------------------------------------------------------------

const static std::filesystem::path TEMP_DIR = "/tmp";

// ---------------------------------------------------------------------

const static std::string LOREM_IPSUM =
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

BOOST_AUTO_TEST_CASE( temporary )
{
    std::filesystem::path path;

    {
        temp_file tf(TEMP_DIR);
        path = tf.path();

        BOOST_CHECK(path.parent_path() == TEMP_DIR);
        BOOST_CHECK(std::filesystem::exists(path));
    }

    BOOST_CHECK(!std::filesystem::exists(path));
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( read_write )
{
    temp_file tf(TEMP_DIR);

    auto written = tf.write({LOREM_IPSUM.c_str(), LOREM_IPSUM.size()});
    BOOST_CHECK_EQUAL(written, LOREM_IPSUM.size());

    auto seekpos = tf.seek(0, std::ios_base::beg);
    BOOST_CHECK_EQUAL(seekpos, 0);

    std::string copy(LOREM_IPSUM.size(), 0);
    auto read = tf.read({copy.data(), copy.size()});
    BOOST_CHECK_EQUAL(read, LOREM_IPSUM.size());

    BOOST_CHECK_EQUAL(copy, LOREM_IPSUM);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( throw_if_directory_missing )
{
    unsigned counter = 0;
    std::stringstream subdir;
    std::filesystem::path dir = ".";

    do
    {
        subdir.clear();
        subdir << "tmp-" << counter;
        ++counter;
    }
    while (std::filesystem::exists(dir / subdir.str()));

    BOOST_CHECK_THROW(temp_file(dir / subdir.str()), exception);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( release_to )
{
    auto target_path = TEMP_DIR / "release-to-unit-test";

    std::filesystem::remove(target_path);
    std::filesystem::path tf_path;

    {
        temp_file tf(TEMP_DIR);
        tf_path = tf.path();
        tf.release_to(target_path);

        BOOST_CHECK(std::filesystem::exists(tf_path));
        BOOST_CHECK(std::filesystem::exists(target_path));
    }

    BOOST_CHECK(!std::filesystem::exists(tf_path));
    BOOST_CHECK(std::filesystem::exists(target_path));

    std::filesystem::remove(target_path);
}

// ---------------------------------------------------------------------

} // namespace

