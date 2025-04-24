#define BOOST_TEST_MODULE "deduplicator tests"

#include <common/utils/random.h>
#include <deduplicator/interfaces/local_deduplicator.h>

#include <mock/storage/mock_data_view.h>
#include <util/coroutine.h>
#include <util/temp_directory.h>

#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>

#define MAX_DATA_STORE_SIZE_BYTES (4 * MEBI_BYTE)
#define MAX_FILE_SIZE_BYTES (128 * KIBI_BYTE)
#define DATA_STORE_ID 1

namespace uh::cluster {

class dedup_coro_fixture : public coro_fixture {
public:
    dedup_coro_fixture()
        : coro_fixture(2) {}
};

BOOST_FIXTURE_TEST_CASE(deduplicate, dedup_coro_fixture) {
    auto log_config = log::config{
        .sinks = {log::sink_config{.type = log::sink_type::cout,
                                   .level = boost::log::trivial::fatal,
                                   .service_role = DEDUPLICATOR_SERVICE}}};
    log::init(log_config);

    temp_directory dir;
    auto config =
        data_store_config{.max_file_size = MAX_FILE_SIZE_BYTES,
                          .max_data_store_size = MAX_DATA_STORE_SIZE_BYTES,
                          .page_size = DEFAULT_PAGE_SIZE};
    auto data_store =
        mock_data_store(config, dir.path().string(), DATA_STORE_ID, 0);
    auto data_view = mock_data_view(data_store);
    auto cache = storage::global::cache(get_io_context(), data_view, 4000ul);
    auto dedup = local_deduplicator({}, data_view, cache);

    auto data = random_string(66);

    auto f = [&]() -> coro<dedupe_response> {
        co_return co_await dedup.deduplicate(data);
    };
    {
        std::future<dedupe_response> res = spawn(f);
        auto dedup_response = res.get();
        BOOST_TEST(dedup_response.effective_size == data.size());
        BOOST_TEST(data_store.get_used_space() == data.size());
    }
    {
        std::future<dedupe_response> res = spawn(f);
        auto dedup_response = res.get();
        BOOST_TEST(dedup_response.effective_size == 0);
        BOOST_TEST(data_store.get_used_space() == data.size());
    }
    {
        std::future<dedupe_response> res = spawn(f);
        auto dedup_response = res.get();
        BOOST_TEST(dedup_response.effective_size == 0);
        BOOST_TEST(data_store.get_used_space() == data.size());
    }
}

} // namespace uh::cluster
