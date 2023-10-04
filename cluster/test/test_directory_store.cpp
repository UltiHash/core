//
// Created by masi on 7/24/23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "directory_store tests"
#endif

#include <boost/test/unit_test.hpp>
#include <directory_node/directory_store.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

struct config_fixture
{
    static uh::cluster::bucket_config make_bucket_config () {
        return {
                .min_file_size = 1024ul,
                .max_file_size = 8ul * 1024ul,
                .max_storage_size = 16 * 1024ul,
                .max_chunk_size = 16 * 1024ul,
        };
    }

    static void cleanup () {
        std::filesystem::remove_all("root");
    }
};


// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE (test_chaining_data_store, config_fixture)
{

    cleanup();

    char data1[10];

    {
        directory_store ds ("root", make_bucket_config());
        ds.add_bucket("b1");
        ds.insert("b1", "k1", data1);
    }

    cleanup();
}

// ---------------------------------------------------------------------

} // end namespace uh::cluster
