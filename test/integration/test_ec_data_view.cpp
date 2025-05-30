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

BOOST_AUTO_TEST_CASE(reads_small_data_when_two_storages_are_down) {
    auto gdv = get_data_view();
    auto buffer = random_buffer(64);

    LOG_DEBUG() << "write data: " << buffer.string_view();
    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    LOG_DEBUG() << "kill storage 1";
    stop_storage(1);

    LOG_DEBUG() << "kill storage 4";
    stop_storage(4);

    auto read_buffer = shared_buffer<char>(buffer.size());

    LOG_DEBUG() << "start reading...";
    auto read_size =
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, read_buffer.span()),
                              boost::asio::use_future)
            .get();

    BOOST_TEST(buffer.size() == read_size);
    LOG_DEBUG() << "buffer: " << buffer.string_view();
    LOG_DEBUG() << "read_buffer: " << read_buffer.string_view();

    BOOST_TEST(buffer == read_buffer);
}

BOOST_AUTO_TEST_CASE(reads_one_and_half_stripes_when_two_storages_are_down) {
    auto config = get_group_config();
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 1.5);

    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    LOG_DEBUG() << "kill storage 1";
    stop_storage(1);

    LOG_DEBUG() << "kill storage 4";
    stop_storage(4);

    auto read_buffer = shared_buffer<char>(buffer.size());

    LOG_DEBUG() << "start reading...";
    auto read_size =
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, read_buffer.span()),
                              boost::asio::use_future)
            .get();

    BOOST_TEST(buffer.size() == read_size);
    LOG_DEBUG() << "buffer: " << buffer.string_view();
    LOG_DEBUG() << "read_buffer: " << read_buffer.string_view();

    BOOST_TEST(buffer == read_buffer);
}

BOOST_AUTO_TEST_CASE(reads_two_stripes_when_two_storages_are_down) {
    auto config = get_group_config();
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * 2);

    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    LOG_DEBUG() << "kill storage 1";
    stop_storage(1);

    LOG_DEBUG() << "kill storage 4";
    stop_storage(4);

    auto read_buffer = shared_buffer<char>(buffer.size());

    LOG_DEBUG() << "start reading...";
    auto read_size =
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, read_buffer.span()),
                              boost::asio::use_future)
            .get();

    BOOST_TEST(buffer.size() == read_size);
    LOG_DEBUG() << "buffer: " << buffer.string_view();
    LOG_DEBUG() << "read_buffer: " << read_buffer.string_view();

    BOOST_TEST(buffer == read_buffer);
}

BOOST_AUTO_TEST_CASE(write_chunk_fragmentation_full) {
    auto& etcd = get_etcd_manager();
    auto config = get_group_config();
    etcd.wait(ns::root.storage_groups[config.id].group_state);
    auto gdv = get_data_view();
    auto buffer = random_buffer(config.stripe_size_kib * KIBI_BYTE * 2);

    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();
    auto chunk_size = (config.stripe_size_kib * KIBI_BYTE) / config.data_shards;
    auto num_chunks = buffer.size() / chunk_size;
    BOOST_TEST(addr.size() == num_chunks);
}

BOOST_AUTO_TEST_CASE(write_chunk_fragmentation_padded) {
    auto& etcd = get_etcd_manager();
    auto config = get_group_config();
    etcd.wait(ns::root.storage_groups[config.id].group_state);
    auto gdv = get_data_view();
    auto chunk_size = (config.stripe_size_kib * KIBI_BYTE) / config.data_shards;
    auto buffer = random_buffer(chunk_size);

    auto addr = boost::asio::co_spawn(get_executor(),
                                      gdv->write(buffer.string_view(), {0}),
                                      boost::asio::use_future)
                    .get();

    auto num_chunks = buffer.size() / chunk_size;
    BOOST_TEST(addr.size() == num_chunks);
}

// BOOST_AUTO_TEST_CASE(reads_when_one_storage_is_down) {
//     //
// }
//
BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
