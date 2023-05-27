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
#include <io/temp_file.h>
#include <io/chunk_collection.h>
#include <io/buffer.h>
#include <io/device.h>

#include <boost/test/unit_test.hpp>

#include <filesystem>
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

        {
            chunk_collection<uh::io::temp_file> temporary_chunk_collection2(TEMP_DIR);
            temporary_chunk_collection2.release_to(temporary_chunk_collection2.getPath());
            work_path = temporary_chunk_collection2.getPath();
        }
        BOOST_REQUIRE(std::filesystem::exists(work_path));

        chunk_collection<> file_chunk_collection(work_path);

        std::filesystem::remove(work_path);
    }

// ---------------------------------------------------------------------

    BOOST_AUTO_TEST_CASE( write_read_chunk_collection )
    {
        chunk_collection<uh::io::temp_file> cc(TEMP_DIR);

        for(uint16_t i = 0; i < std::numeric_limits<uint8_t>::max()+1; i++){
            std::string input = uh::test::LOREM_IPSUM + std::to_string(i);

            if(i == std::numeric_limits<uint8_t>::max()){
                BOOST_REQUIRE_THROW(cc.write_indexed(input),uh::util::exception);
                break;
            }
            else
                cc.write_indexed(input);

            auto read_back = cc.read_indexed(i);
            BOOST_REQUIRE_EQUAL_COLLECTIONS(input.begin(),input.end(),
                                            read_back.first.begin(),read_back.first.end());
        }

        std::string input3 = uh::test::LOREM_IPSUM + std::to_string(3);
        auto read_back3 = cc.read_indexed(3);

        BOOST_CHECK_EQUAL_COLLECTIONS(input3.begin(),input3.end(),
                                        read_back3.first.begin(),read_back3.first.end());
    }

// ---------------------------------------------------------------------

} // namespace

