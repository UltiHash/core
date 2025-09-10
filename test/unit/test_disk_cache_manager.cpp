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
    reader_body rbody(data_view);

    boost::asio::co_spawn(
        m_ioc, rbody.put(std::span<const char>(data.data(), data.size())),
        boost::asio::use_future)
        .get();

    object_metadata key;
    key.path = "/foo/bar";
    key.version = "v1";

    boost::asio::co_spawn(m_ioc, mgr.put(key, rbody), boost::asio::use_future)
        .get();

    auto writer = mgr.get(key);
    BOOST_TEST(writer != nullptr);

    auto buf =
        boost::asio::co_spawn(m_ioc, writer->get(), boost::asio::use_future)
            .get();

    BOOST_TEST(buf.size() == data.size());
    BOOST_TEST(std::string(buf.data(), buf.size()) == data);
}

BOOST_AUTO_TEST_CASE(eviction_test) {
    manager mgr{manager::create(m_ioc, data_view, 256)};

    std::vector<object_metadata> keys;
    std::vector<std::string> datas;

    for (int i = 0; i < 10; ++i) {
        std::string data = random_string(32);
        datas.push_back(data);

        reader_body rbody(data_view);
        boost::asio::co_spawn(
            m_ioc, rbody.put(std::span<const char>(data.data(), data.size())),
            boost::asio::use_future)
            .get();

        object_metadata key;
        key.path = "/evict/" + std::to_string(i);
        key.version = "v" + std::to_string(i);
        keys.push_back(key);

        boost::asio::co_spawn(m_ioc, mgr.put(key, rbody),
                              boost::asio::use_future)
            .get();
    }

    auto writer = mgr.get(keys.front());
    BOOST_TEST(writer == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::proxy::cache::disk
