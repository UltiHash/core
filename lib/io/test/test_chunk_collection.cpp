//
// Created by benjamin-elias on 27.05.23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibUtil ChunkCollection Tests"
#endif

#include <test/ipsum.h>
#include <util/exception.h>
#include <io/file.h>
#include <io/temp_file.h>
#include <io/chunk_collection.h>

#include <boost/test/unit_test.hpp>

#include <numeric>
#include <limits>

using namespace uh::util;
using namespace uh::io;
using namespace uh::test;

namespace
{

// ---------------------------------------------------------------------

    const static std::filesystem::path TEMP_DIR = "/tmp";

// ---------------------------------------------------------------------

    BOOST_AUTO_TEST_CASE( temporary_chunk_collecion )
    {
        std::filesystem::path work_path;

        {
            chunk_collection<uh::io::temp_file> temporary_chunk_collection(TEMP_DIR);
            work_path = temporary_chunk_collection.getPath();
            BOOST_REQUIRE(std::filesystem::exists(work_path));
        }
        BOOST_REQUIRE(!std::filesystem::exists(work_path));
    }

// ---------------------------------------------------------------------

} // namespace

