#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibIo File Tests"
#endif

#include <io/file.h>
#include <util/temp_dir.h>
#include <test/ipsum.h>

#include <boost/test/unit_test.hpp>


using namespace uh::util;
using namespace uh::io;

namespace
{

// ---------------------------------------------------------------------

const std::filesystem::path TEMP_DIR_NONEXIST = "tmp/testfile";

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( no_path_throw )
{
    temp_directory temp_dir;
    const std::filesystem::path& path = temp_dir.path();

    BOOST_REQUIRE_THROW(uh::io::file(path/TEMP_DIR_NONEXIST).write(uh::test::LOREM_IPSUM),std::exception);
}

// ---------------------------------------------------------------------

} // namespace

