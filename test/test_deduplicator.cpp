#define BOOST_TEST_MODULE "deduplicator tests"

#include "common/test/coroutine.h"
#include "common/utils/temp_directory.h"
#include "deduplicator/interfaces/local_deduplicator.h"
#include "fakes/storage/fake_global_data_view.h"
#include "utils/random_string.h"

#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>

#define MAX_DATA_STORE_SIZE_BYTES (4 * MEBI_BYTE)
#define MAX_FILE_SIZE_BYTES (128 * KIBI_BYTE)
#define DATA_STORE_ID 1

namespace uh::cluster {

BOOST_FIXTURE_TEST_CASE(deduplicate, coro_fixture) {

    temp_directory dir;
    auto config =
        data_store_config{.max_file_size = MAX_FILE_SIZE_BYTES,
                          .max_data_store_size = MAX_DATA_STORE_SIZE_BYTES,
                          .page_size = DEFAULT_PAGE_SIZE};
    auto data_store =
        fake_data_store(config, dir.path().string(), DATA_STORE_ID, 0);
    auto data_view = fake_global_data_view(get_io_context(), data_store);
    auto dedup = local_deduplicator({}, data_view);

    context ctx;
    auto data = generate_random_string(7);

    auto f = [&]() -> coro<dedupe_response> {
        co_return co_await dedup.deduplicate(ctx, data);
    };
    {
        std::future<dedupe_response> res = spawn(f);
        auto dedup_response = res.get();
        BOOST_TEST(dedup_response.effective_size == data.size());
    }
    {
        std::future<dedupe_response> res = spawn(f);
        auto dedup_response = res.get();
        BOOST_TEST(dedup_response.effective_size == data.size());
    }
    {
        std::future<dedupe_response> res = spawn(f);
        auto dedup_response = res.get();
        BOOST_TEST(dedup_response.effective_size == data.size());
    }
}

} // namespace uh::cluster
