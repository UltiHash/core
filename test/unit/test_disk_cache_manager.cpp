#define BOOST_TEST_MODULE "disk-cache manager tests"

#include <boost/test/unit_test.hpp>
#include <chrono>
#include <common/utils/random.h>
#include <proxy/cache/disk/manager.h>
#include <thread>
#include <util/dedupe_fixture.h>

namespace uh::cluster::proxy::cache::disk {

BOOST_FIXTURE_TEST_SUITE(a_disk_cache_manager, dedupe_fixture)

BOOST_AUTO_TEST_CASE(put_and_get_with_metadata) {
    manager mgr{manager::create(m_ioc, data_view, 256)};

    std::string data = random_string(64);
    disk_sink sink(data_view);

    boost::asio::co_spawn(
        m_ioc, sink.put(std::span<const char>(data.data(), data.size())),
        boost::asio::use_future)
        .get();

    object_metadata key;
    key.path = "/foo/bar";
    key.version = "v1";

    boost::asio::co_spawn(m_ioc, mgr.put(key, sink), boost::asio::use_future)
        .get();

    auto source = mgr.get(key);
    BOOST_TEST(source != nullptr);

    auto buf = std::string(128, '\0');
    auto sv =
        boost::asio::co_spawn(m_ioc, source->get(buf), boost::asio::use_future)
            .get();

    BOOST_TEST(sv.size() == data.size());
    BOOST_TEST(std::string(sv.data(), sv.size()) == data);
}

BOOST_AUTO_TEST_CASE(eviction_test) {
    manager mgr{manager::create(m_ioc, data_view, 256)};

    std::vector<object_metadata> keys;
    std::vector<std::string> datas;

    for (int i = 0; i < 10; ++i) {
        std::string data = random_string(32);
        datas.push_back(data);

        disk_sink sink(data_view);
        boost::asio::co_spawn(
            m_ioc, sink.put(std::span<const char>(data.data(), data.size())),
            boost::asio::use_future)
            .get();

        object_metadata key;
        key.path = "/evict/" + std::to_string(i);
        key.version = "v" + std::to_string(i);
        keys.push_back(key);

        boost::asio::co_spawn(m_ioc, mgr.put(key, sink),
                              boost::asio::use_future)
            .get();
    }

    auto source = mgr.get(keys.front());
    BOOST_TEST(source == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::proxy::cache::disk
