#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibUtil TempFile Tests"
#endif

#include <test/ipsum.h>
#include <util/exception.h>
#include <io/file.h>
#include <io/temp_file.h>

#include <boost/test/unit_test.hpp>


using namespace uh::util;
using namespace uh::io;
using namespace uh::test;

namespace
{

// ---------------------------------------------------------------------

const static std::filesystem::path TEMP_DIR = "/tmp";

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
    std::filesystem::path test_path;
    {
        temp_file tf(TEMP_DIR);
        test_path = tf.path();

        auto written = tf.write({LOREM_IPSUM.c_str(), LOREM_IPSUM.size()});
        BOOST_CHECK_EQUAL(written, LOREM_IPSUM.size());

        tf.release_to(test_path);
    }

    file in(test_path,std::ios_base::in);
    std::string copy(LOREM_IPSUM.size(), 0);
    auto read = in.read({copy.data(), copy.size()});
    BOOST_CHECK_EQUAL(read, LOREM_IPSUM.size());

    BOOST_CHECK_EQUAL(copy, LOREM_IPSUM);

    if(std::filesystem::exists(test_path))
        std::filesystem::remove(test_path);
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

        BOOST_CHECK(std::filesystem::exists(target_path));
    }

    BOOST_CHECK(!std::filesystem::exists(tf_path));
    BOOST_CHECK(std::filesystem::exists(target_path));

    std::filesystem::remove(target_path);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( release_to_throws )
{
    temp_file tf_1(TEMP_DIR);
    temp_file tf_2(TEMP_DIR);

    BOOST_CHECK_THROW(tf_1.release_to(tf_2.path()), file_exists);
}

// ---------------------------------------------------------------------

} // namespace

