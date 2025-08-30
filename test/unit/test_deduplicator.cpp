#define BOOST_TEST_MODULE "deduplicator tests"

#include <boost/test/unit_test.hpp>

#include <common/utils/random.h>
#include <util/dedupe_fixture.h>

using namespace uh::cluster;

BOOST_FIXTURE_TEST_CASE(deduplicate, dedupe_fixture) {

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
