#define BOOST_TEST_MODULE "ec tests"

#include <common/ec/reedsolomon_c.h>
#include <common/telemetry/log.h>
#include <common/types/common_types.h>
#include <common/utils/time_utils.h>
#include <util/gdv_fixture.h>
#include <util/random.h>

#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

struct fixture : public global_data_view_fixture {

    fixture()
        : global_data_view_fixture({
              .id = 0,
              .type = storage::group_config::type_t::ERASURE_CODING,
              .storages = 6,
              .data_shards = 4,
              .parity_shards = 2,
              .stripe_size_kib = 4 * 2,
          }) {}
};

BOOST_FIXTURE_TEST_SUITE(a_ec_data_view, fixture)

BOOST_AUTO_TEST_CASE(reads_small_data_on_degraded_state) {
    auto gdv = get_data_view();
    auto buffer = random_buffer(64);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    LOG_DEBUG() << "write data: " << buffer.string_view();
    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    LOG_DEBUG() << "kill storage 1";
    deactivate_storage(1);

    LOG_DEBUG() << "kill storage 4";
    deactivate_storage(4);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::DEGRADED));

    auto read_buffer = shared_buffer<char>(buffer.size());

    LOG_DEBUG() << "start reading...";
    auto read_size =
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, read_buffer.span()),
                              boost::asio::use_future)
            .get();

    BOOST_TEST(buffer.size() == read_size);
    BOOST_TEST(buffer == read_buffer);
}

BOOST_AUTO_TEST_CASE(writes_returns_correct_address) {
    auto config = get_group_config();
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB + 1);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    auto& before = addr.fragments[0];
    auto size = before.size;
    for (auto& current : addr.fragments | std::views::drop(1)) {
        BOOST_TEST(current.pointer == before.pointer + before.size);
        before = current;
        size += current.size;
    }
    BOOST_TEST(buffer.size() == size);
}

BOOST_AUTO_TEST_CASE(reads_one_stripe_and_more_data_on_degraded_state) {
    auto config = get_group_config();
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB + 1);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    LOG_DEBUG() << "kill storage 1";
    deactivate_storage(1);

    LOG_DEBUG() << "kill storage 4";
    deactivate_storage(4);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::DEGRADED));

    auto read_buffer = shared_buffer<char>(buffer.size());

    LOG_DEBUG() << "start reading...";
    auto read_size =
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, read_buffer.span()),
                              boost::asio::use_future)
            .get();

    BOOST_TEST(buffer.size() == read_size);
    BOOST_TEST(buffer == read_buffer);
}

BOOST_AUTO_TEST_CASE(reads_two_stripes_on_degraded_state) {
    auto config = get_group_config();
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB * 2);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    LOG_DEBUG() << "kill storage 1";
    deactivate_storage(1);

    LOG_DEBUG() << "kill storage 4";
    deactivate_storage(4);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::DEGRADED));

    auto read_buffer = shared_buffer<char>(buffer.size());

    LOG_DEBUG() << "start reading...";
    auto read_size =
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, read_buffer.span()),
                              boost::asio::use_future)
            .get();

    BOOST_TEST(buffer.size() == read_size);
    BOOST_TEST(buffer == read_buffer);
}

BOOST_AUTO_TEST_CASE(fails_to_write_on_degraded_state) {
    auto config = get_group_config();
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB * 2);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    LOG_DEBUG() << "kill storage 1";
    deactivate_storage(1);

    LOG_DEBUG() << "kill storage 4";
    deactivate_storage(4);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::DEGRADED));

    BOOST_REQUIRE_THROW(
        boost::asio::co_spawn(get_executor(),
                              gdv->write(buffer.string_view(), {0}),
                              boost::asio::use_future)
            .get(),
        std::runtime_error);
}

BOOST_AUTO_TEST_CASE(reads_after_transition_from_degraded_to_healthy_state) {
    auto config = get_group_config();
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB * 2);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    LOG_DEBUG() << "kill storage 1";
    deactivate_storage(1);

    LOG_DEBUG() << "kill storage 4";
    deactivate_storage(4);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::DEGRADED));

    LOG_DEBUG() << "revive storage 1";
    activate_storage(1);

    LOG_DEBUG() << "revive storage 4";
    activate_storage(4);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    auto read_buffer = shared_buffer<char>(buffer.size());

    LOG_DEBUG() << "start reading...";
    auto read_size =
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, read_buffer.span()),
                              boost::asio::use_future)
            .get();

    BOOST_TEST(buffer.size() == read_size);
    BOOST_TEST(buffer == read_buffer);
}

BOOST_AUTO_TEST_CASE(writes_after_transition_from_degraded_to_healthy_state) {
    auto config = get_group_config();
    auto gdv = get_data_view();

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    {
        auto buffer = random_buffer(config.stripe_size_kib * 1_KiB * 2);

        auto addr = boost::asio::co_spawn(get_executor(),
                                          gdv->write(buffer.string_view(), {0}),
                                          boost::asio::use_future)
                        .get();
    }

    LOG_DEBUG() << "kill storage 1";
    deactivate_storage(1);

    LOG_DEBUG() << "kill storage 4";
    deactivate_storage(4);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::DEGRADED));

    LOG_DEBUG() << "revive storage 1";
    activate_storage(1);

    LOG_DEBUG() << "revive storage 4";
    activate_storage(4);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB + 2);

    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    auto read_buffer = shared_buffer<char>(buffer.size());

    LOG_DEBUG() << "start reading...";
    auto read_size =
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, read_buffer.span()),
                              boost::asio::use_future)
            .get();

    BOOST_TEST(buffer.size() == read_size);
    BOOST_TEST(buffer == read_buffer);
}

BOOST_AUTO_TEST_CASE(fails_to_read_on_failed_state) {
    auto config = get_group_config();
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB * 2);
    std::cout << "buffer size: " << buffer.string_view().size();

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    std::cout << "address size: " << addr.size() << std::endl;

    LOG_DEBUG() << "kill storage 1";
    deactivate_storage(1);

    LOG_DEBUG() << "kill storage 4";
    deactivate_storage(4);

    LOG_DEBUG() << "kill storage 5";
    deactivate_storage(5);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::FAILED));

    auto read_buffer = shared_buffer<char>(buffer.size());

    LOG_DEBUG() << "start reading...";
    BOOST_REQUIRE_THROW(
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, read_buffer.span()),
                              boost::asio::use_future)
            .get(),
        std::runtime_error);
}

BOOST_AUTO_TEST_CASE(reads_after_transition_from_failed_to_degraded_state) {
    auto config = get_group_config();
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB * 2);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    LOG_DEBUG() << "write starting...";
    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    LOG_DEBUG() << "kill storage 1";
    deactivate_storage(1);

    LOG_DEBUG() << "kill storage 4";
    deactivate_storage(4);

    LOG_DEBUG() << "kill storage 5";
    deactivate_storage(5);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::FAILED));

    LOG_DEBUG() << "revive storage 5";
    activate_storage(5);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::DEGRADED));

    auto read_buffer = shared_buffer<char>(buffer.size());

    LOG_DEBUG() << "start reading...";
    auto read_size =
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, read_buffer.span()),
                              boost::asio::use_future)
            .get();

    BOOST_TEST(buffer.size() == read_size);
    BOOST_TEST(buffer == read_buffer);
}

BOOST_AUTO_TEST_CASE(writes_after_transition_from_failed_to_healthy_state) {
    auto config = get_group_config();
    auto gdv = get_data_view();

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    {
        auto buffer = random_buffer(config.stripe_size_kib * 1_KiB * 2);
        LOG_DEBUG() << "write starting...";
        auto addr = boost::asio::co_spawn(get_executor(),
                                          gdv->write(buffer.string_view(), {0}),
                                          boost::asio::use_future)
                        .get();
    }

    LOG_DEBUG() << "kill storage 1";
    deactivate_storage(1);

    LOG_DEBUG() << "kill storage 4";
    deactivate_storage(4);

    LOG_DEBUG() << "kill storage 5";
    deactivate_storage(5);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::FAILED));

    LOG_DEBUG() << "revive storage 1";
    activate_storage(1);

    LOG_DEBUG() << "revive storage 4";
    activate_storage(4);

    LOG_DEBUG() << "revive storage 5";
    activate_storage(5);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB + 2);
    LOG_DEBUG() << "write starting...";
    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    auto read_buffer = shared_buffer<char>(buffer.size());

    LOG_DEBUG() << "start reading...";
    auto read_size =
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, read_buffer.span()),
                              boost::asio::use_future)
            .get();

    BOOST_TEST(buffer.size() == read_size);
    BOOST_TEST(buffer == read_buffer);
}

BOOST_AUTO_TEST_CASE(reads_after_transition_from_degraded_to_repairing_state) {
    auto config = get_group_config();
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB * 2);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    LOG_DEBUG() << "write starting...";
    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    LOG_DEBUG() << "kill storage 1";
    deactivate_storage(1);

    LOG_DEBUG() << "kill storage 5";
    deactivate_storage(5);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::DEGRADED));

    LOG_DEBUG() << "introduce new storage as storage 1";
    introduce_new_storage(1);

    LOG_DEBUG() << "introduce new storage as storage 5";
    introduce_new_storage(5);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::REPAIRING));

    auto read_buffer = shared_buffer<char>(buffer.size());

    LOG_DEBUG() << "start reading...";
    auto read_size =
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, read_buffer.span()),
                              boost::asio::use_future)
            .get();

    BOOST_TEST(buffer.size() == read_size);
    BOOST_TEST(buffer == read_buffer);
}

BOOST_AUTO_TEST_CASE(write_chunk_fragmentation_full) {
    auto& etcd = get_etcd_manager();
    auto config = get_group_config();
    etcd.wait(ns::root.storage_groups[config.id].group_state);
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 1_KiB * 2);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();
    auto chunk_size = (config.stripe_size_kib * 1_KiB) / config.data_shards;
    auto num_chunks = buffer.size() / chunk_size;
    BOOST_TEST(addr.size() == num_chunks);

    auto read_buffer = shared_buffer<char>(buffer.size());

    LOG_DEBUG() << "start reading...";
    auto read_size =
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, read_buffer.span()),
                              boost::asio::use_future)
            .get();

    BOOST_TEST(buffer.size() == read_size);
    BOOST_TEST(buffer == read_buffer);
}

BOOST_AUTO_TEST_CASE(write_chunk_fragmentation_padded) {
    auto& etcd = get_etcd_manager();
    auto config = get_group_config();
    etcd.wait(ns::root.storage_groups[config.id].group_state);
    auto gdv = get_data_view();
    auto chunk_size = (config.stripe_size_kib * 1_KiB) / config.data_shards;
    auto buffer = random_buffer(chunk_size);

    get_etcd_manager().wait(
        ns::root.storage_groups[get_group_config().id].group_state,
        serialize(storage::group_state::HEALTHY));

    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    auto num_chunks = buffer.size() / chunk_size;
    BOOST_TEST(addr.size() == num_chunks);

    auto read_buffer = shared_buffer<char>(buffer.size());

    LOG_DEBUG() << "start reading...";
    auto read_size =
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, read_buffer.span()),
                              boost::asio::use_future)
            .get();

    BOOST_TEST(buffer.size() == read_size);
    BOOST_TEST(buffer == read_buffer);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
