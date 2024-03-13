#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "bucket tests"
#endif

#include "common/utils/temp_directory.h"
#include "directory/bucket.h"
#include "storage/data_store.h"
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

#define BUCKET_NAME "bucket1"
#define KEY_NAME "key1"

struct config_fixture {
    static uh::cluster::bucket_config get_bucket_config() {
        return {
            .min_file_size = 1024ul,
            .max_file_size = 8ul * 1024ul,
            .max_storage_size = 16 * 1024ul,
            .max_chunk_size = 16 * 1024ul,
        };
    }

    data_store_config make_data_store_config() const {
        return {.working_dir = dir.path().string(),
                .min_file_size = 1024ul,
                .max_file_size = 8 * 1024ul,
                .max_data_store_size = 32 * 1024ul};
    }

    void setup() {
        ds = std::make_unique<data_store>(make_data_store_config(), 0);
        bt = std::make_unique<bucket>(dir.path(), BUCKET_NAME,
                                      get_bucket_config());

        char data1[] = "Would be a shame if we lost this data!";
        original_addr = ds->write(data1);

        bt->insert_object(KEY_NAME, original_addr);
    }

    std::unique_ptr<data_store> ds;
    std::unique_ptr<bucket> bt;
    address original_addr;
    temp_directory dir;
};

static bool compare_address(const auto& addr1, const auto& addr2) {
    for (size_t iteration = 0; iteration < addr1.pointers.size(); iteration++) {
        if (addr1.pointers.at(iteration) != addr2.pointers.at(iteration) ||
            addr1.sizes.at(iteration / 2) != addr2.sizes.at(iteration / 2)) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(test_transaction_replay, config_fixture) {
    bt.reset();
    bt = std::make_unique<bucket>(dir.path(), BUCKET_NAME, get_bucket_config());

    auto address_recv = bt->get_obj("key1");
    BOOST_CHECK(compare_address(original_addr, address_recv));
}

// ---------------------------------------------------------------------

} // end namespace uh::cluster
