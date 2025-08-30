#define BOOST_TEST_MODULE "disk-cache's utils tests"

#include <boost/test/unit_test.hpp>

#include <proxy/cache/disk/utils.h>

#include <common/utils/random.h>
#include <util/dedupe_fixture.h>

namespace uh::cluster::proxy::cache::disk {

BOOST_FIXTURE_TEST_SUITE(a_disk_cache_utils, dedupe_fixture)

BOOST_AUTO_TEST_CASE(does_CRD) {
    auto data = random_string(66);

    auto f = [&]() -> coro<void> {
        auto addr = co_await utils::store(
            dedup, std::span<const char>{data.data(), data.size()});
        BOOST_TEST(addr.data_size() == data.size());

        std::vector<char> buf(data.size());
        co_await utils::read(data_view, addr,
                             std::span<char>{buf.data(), buf.size()});
        BOOST_TEST(std::string(buf.data(), buf.size()) == data);

        co_await utils::erase(data_view, addr);
        co_await utils::read(data_view, addr,
                             std::span<char>{buf.data(), buf.size()});
        BOOST_TEST(std::string(buf.data(), buf.size()) != data);
    };
    {
        std::future<void> res = spawn(f);
        res.get();
    }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::proxy::cache::disk
