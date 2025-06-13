#define BOOST_TEST_MODULE "test_service_outage"

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <common/etcd/utils.h>
#include <common/utils/common.h>
#include <random>
#include <thread>
#include <util/gdv_fixture_using_process.h>
#include <util/random.h>

#include "test_config.h"

namespace bp = boost::process;
using namespace uh::cluster;

struct fixture : public gdv_fixture_using_process {

    fixture()
        : gdv_fixture_using_process({
              .id = 0,
              .type = storage::group_config::type_t::ERASURE_CODING,
              .storages = 6,
              .data_shards = 4,
              .parity_shards = 2,
              .stripe_size_kib = 4 * 2,
          }) {

        // To make test faster,
        time_settings::instance().set_async_io_timeout(1s);
    }
};

BOOST_FIXTURE_TEST_SUITE(a_storage_quorum, fixture)

BOOST_AUTO_TEST_CASE(
    supports_read_when_storage_is_killed_and_has_been_down_for_a_long_time) {
    auto config = get_group_config();
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB * 2);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    LOG_DEBUG() << "start writing...";
    address addr;
    BOOST_REQUIRE_NO_THROW({
        addr = boost::asio::co_spawn(get_executor(),
                                     gdv->write(buffer.string_view(), {0}),
                                     boost::asio::use_future)
                   .get();
    });

    std::size_t idx = 0;
    LOG_DEBUG() << "Kill storage " << idx;
    kill_storage(idx);

    constexpr auto test_time = 7s;
    std::size_t read_size;
    auto read_buffer = shared_buffer<char>(buffer.size());

    auto end_time = std::chrono::steady_clock::now() + test_time;
    std::size_t cnt = 0;
    while (std::chrono::steady_clock::now() < end_time) {
        LOG_DEBUG() << "Read loop..." << ++cnt;
        BOOST_REQUIRE_NO_THROW({
            read_size =
                boost::asio::co_spawn(
                    get_executor(), gdv->read_address(addr, read_buffer.span()),
                    boost::asio::use_future)
                    .get();
        });
        BOOST_TEST(buffer.size() == read_size);
        BOOST_TEST(buffer == read_buffer);
    }
}

BOOST_AUTO_TEST_CASE(supports_read_during_deactivating_reactivating_storages) {
    auto config = get_group_config();
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB * 2);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    LOG_DEBUG() << "start writing...";
    address addr;
    BOOST_REQUIRE_NO_THROW({
        addr = boost::asio::co_spawn(get_executor(),
                                     gdv->write(buffer.string_view(), {0}),
                                     boost::asio::use_future)
                   .get();
    });

    constexpr auto test_time = 5s;
    std::thread outage_thread(
        [&]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dist(0, config.data_shards - 1);
            std::size_t idx = dist(gen);
            auto end_time = std::chrono::steady_clock::now() + test_time;
            while (std::chrono::steady_clock::now() < end_time) {
                LOG_DEBUG()
                    << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
                LOG_DEBUG() << "Deactivate storage " << idx;
                deactivate_storage(idx);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                LOG_DEBUG()
                    << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
                LOG_DEBUG() << "Revive storage " << idx;
                activate_storage(idx);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            LOG_DEBUG() << "Outage thread finished";
        });

    std::this_thread::yield();

    std::size_t read_size;
    auto read_buffer = shared_buffer<char>(buffer.size());

    auto end_time = std::chrono::steady_clock::now() + test_time;
    while (std::chrono::steady_clock::now() < end_time) {
        LOG_DEBUG() << "Read loop...";
        BOOST_REQUIRE_NO_THROW({
            read_size =
                boost::asio::co_spawn(
                    get_executor(), gdv->read_address(addr, read_buffer.span()),
                    boost::asio::use_future)
                    .get();
        });
        BOOST_TEST(buffer.size() == read_size);
        BOOST_TEST(buffer == read_buffer);
    }

    outage_thread.join();
}

BOOST_AUTO_TEST_CASE(supports_read_during_killing_reactivating_storages) {
    auto config = get_group_config();
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB * 2);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    LOG_DEBUG() << "start writing...";
    address addr;
    BOOST_REQUIRE_NO_THROW({
        addr = boost::asio::co_spawn(get_executor(),
                                     gdv->write(buffer.string_view(), {0}),
                                     boost::asio::use_future)
                   .get();
    });

    constexpr auto test_time = 5s;
    std::thread outage_thread(
        [&]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dist(0, config.data_shards - 1);
            std::size_t idx = dist(gen);
            auto end_time = std::chrono::steady_clock::now() + test_time;
            while (std::chrono::steady_clock::now() < end_time) {
                LOG_DEBUG()
                    << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
                LOG_DEBUG() << "Kill storage " << idx;
                kill_storage(idx);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                LOG_DEBUG()
                    << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
                LOG_DEBUG() << "Revive storage " << idx;
                activate_storage(idx);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            LOG_DEBUG() << "Outage thread finished";
        });

    std::this_thread::yield();

    std::size_t read_size;
    auto read_buffer = shared_buffer<char>(buffer.size());

    auto end_time = std::chrono::steady_clock::now() + test_time;
    std::size_t cnt = 0;
    while (std::chrono::steady_clock::now() < end_time) {
        LOG_DEBUG() << "Read loop..." << ++cnt;
        BOOST_REQUIRE_NO_THROW({
            read_size =
                boost::asio::co_spawn(
                    get_executor(), gdv->read_address(addr, read_buffer.span()),
                    boost::asio::use_future)
                    .get();
        });
        BOOST_TEST(buffer.size() == read_size);
        BOOST_TEST(buffer == read_buffer);
    }

    outage_thread.join();
}

BOOST_AUTO_TEST_CASE(supports_read_during_slow_killing_reactivating_storages) {
    auto config = get_group_config();
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB * 2);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    LOG_DEBUG() << "start writing...";
    address addr;
    BOOST_REQUIRE_NO_THROW({
        addr = boost::asio::co_spawn(get_executor(),
                                     gdv->write(buffer.string_view(), {0}),
                                     boost::asio::use_future)
                   .get();
    });

    constexpr auto test_time = 5s;
    std::thread outage_thread(
        [&]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dist(0, config.data_shards - 1);
            std::size_t idx = dist(gen);
            auto end_time = std::chrono::steady_clock::now() + test_time;
            while (std::chrono::steady_clock::now() < end_time) {
                LOG_DEBUG()
                    << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
                LOG_DEBUG() << "Kill storage " << idx;
                kill_storage(idx);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                LOG_DEBUG()
                    << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
                LOG_DEBUG() << "Revive storage " << idx;
                activate_storage(idx);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            LOG_DEBUG() << "Outage thread finished";
        });

    std::this_thread::yield();

    std::size_t read_size;
    auto read_buffer = shared_buffer<char>(buffer.size());

    auto end_time = std::chrono::steady_clock::now() + test_time;
    std::size_t cnt = 0;
    while (std::chrono::steady_clock::now() < end_time) {
        LOG_DEBUG() << "Read loop..." << ++cnt;
        BOOST_REQUIRE_NO_THROW({
            read_size =
                boost::asio::co_spawn(
                    get_executor(), gdv->read_address(addr, read_buffer.span()),
                    boost::asio::use_future)
                    .get();
        });
        BOOST_TEST(buffer.size() == read_size);
        BOOST_TEST(buffer == read_buffer);
    }

    outage_thread.join();
}

BOOST_AUTO_TEST_SUITE_END()
