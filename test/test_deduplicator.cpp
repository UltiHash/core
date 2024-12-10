#define BOOST_TEST_MODULE "deduplicator tests"

#include "common/utils/temp_directory.h"
#include "deduplicator/interfaces/local_deduplicator.h"
#include "test_doubles/fake/storage/fake_global_data_view.h"

#include <boost/test/unit_test.hpp>

#define MAX_DATA_STORE_SIZE_BYTES (4 * MEBI_BYTE)
#define MAX_FILE_SIZE_BYTES (128 * KIBI_BYTE)
#define DATA_STORE_ID 1

namespace uh::cluster {

BOOST_AUTO_TEST_CASE(test_used_and_available_space) {

    auto ioc = boost::asio::io_context(1);
    temp_directory dir;
    auto config =
        data_store_config{.max_file_size = MAX_FILE_SIZE_BYTES,
                          .max_data_store_size = MAX_DATA_STORE_SIZE_BYTES,
                          .page_size = DEFAULT_PAGE_SIZE};
    auto data_store =
        fake_data_store(config, dir.path().string(), DATA_STORE_ID, 0);
    auto data_view = fake_global_data_view(ioc, data_store);
    auto dedup = local_deduplicator({}, data_view);
}

} // namespace uh::cluster
