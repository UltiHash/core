//
// Created by masi on 7/24/23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "bucket tests"
#endif

#include <boost/test/unit_test.hpp>
#include <directory/bucket.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

struct config_fixture
{
    static std::filesystem::path get_root_path() {
        return "_tmp_test/dr/";
    }

    static void cleanup () {
        std::filesystem::remove_all("_tmp_test");
    }

    static void setup_prefilled () {
        std::filesystem::create_directories(get_root_path() / "bucket1");
        chaining_data_store_config ds_conf = {
                .directory = get_root_path() / "bucket1",
                .free_spot_log = get_root_path() / "bucket1/free_spot_log",
                .min_file_size = 1024ul,
                .max_file_size = 8ul * 1024ul,
                .max_storage_size = 16 * 1024ul,
                .max_chunk_size = 16 * 1024u,
                };
        chaining_data_store ds (ds_conf);
        char data1[] = "Would be a shame if we lost this data!";
        auto addr1 = ds.write(data1);

        uh::cluster::transaction_log tl(get_root_path()/"bucket1/transaction_log");
        tl.append("key1", addr1, transaction_log::INSERT_START);
        tl.append("key1", addr1, transaction_log::INSERT_END);

    }

    static void setup_empty () {
        std::filesystem::create_directories(get_root_path() / "bucket1");
        chaining_data_store_config ds_conf = {
                .directory = get_root_path() / "bucket1",
                .free_spot_log = get_root_path() / "bucket1/free_spot_log",
                .min_file_size = 1024ul,
                .max_file_size = 8ul * 1024ul,
                .max_storage_size = 16 * 1024ul,
                .max_chunk_size = 16 * 1024u,
        };
    }

    static uh::cluster::bucket_config get_bucket_config() {
        return {
            .min_file_size = 1024ul,
            .max_file_size = 8ul * 1024ul,
            .max_storage_size = 16 * 1024ul,
            .max_chunk_size = 16 * 1024ul,
        };
    }
};

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE (bucket_prefilled_test, config_fixture)
{
    cleanup();
    setup_prefilled();

    uh::cluster::bucket b(get_root_path(), "bucket1", get_bucket_config());
    auto restored_value = b.get_obj("key1");
    char original_value[] = "Would be a shame if we lost this data!";
    BOOST_CHECK(std::memcmp(original_value, restored_value.data(), restored_value.size()) == 0);

    cleanup();
}

// ---------------------------------------------------------------------

} // end namespace uh::cluster
