#define BOOST_TEST_MODULE "ec_data_view tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <storage/group/ec_data_view.h>

#include <nlohmann/json.hpp>

namespace uh::cluster::storage {

struct fixture {
    fixture() {
        m_etcd.clear_all();
        std::this_thread::sleep_for(100ms);
    }

    boost::asio::io_context m_ioc;
    etcd_manager m_etcd;
    const std::size_t m_group_id{0};
    const std::size_t m_service_connections{1};
    const std::size_t m_data_shards{2};
    const std::size_t m_parity_shards{1};
    const group_config m_config{.id = m_group_id,
                                .type = group_config::type_t::ERASURE_CODING,
                                .storages = m_data_shards + m_parity_shards,
                                .data_shards = m_data_shards,
                                .parity_shards = m_parity_shards,
                                .stripe_size_kib = 20};
    ec_data_view sut{m_ioc, m_etcd, m_group_id, m_config,
                     m_service_connections};
};

BOOST_FIXTURE_TEST_SUITE(a_ec_data_view, fixture)

BOOST_AUTO_TEST_CASE(translates_storage_pointer_to_group_pointer) {
    const auto storage_id = 1;
    const auto storage_ptr = 21_KiB;

    auto group_ptr = sut.group_pointer(storage_id, storage_ptr);

    BOOST_TEST(group_ptr == 51_KiB);
}

BOOST_AUTO_TEST_CASE(translates_group_pointer_to_storage_pointer) {
    auto group_ptr = 31_KiB;

    auto [storage_id, storage_ptr] = sut.storage_pointer(group_ptr);

    BOOST_TEST(storage_id == 1);
    BOOST_TEST(storage_ptr == 11_KiB);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
