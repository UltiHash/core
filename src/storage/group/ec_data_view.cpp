#include "ec_data_view.h"

#include <common/telemetry/log.h>

namespace uh::cluster::storage {
ec_data_view::ec_data_view(boost::asio::io_context& ioc, etcd_manager& etcd,
                           std::size_t group_id, group_config group_config,
                           std::size_t service_connections)
    : m_ioc(ioc),
      m_externals(
          etcd, group_id, group_config.storages,
          service_factory<storage_interface>(ioc, service_connections)) {}

coro<address> ec_data_view::write(std::span<const char> data,
                                  const std::vector<std::size_t>& offsets) {
    // TODO: Implement this using
    // - m_externals.get_storage_services();
    // - m_externals.get_storage_states()
    // - m_externals.get_group_state()
    // - m_externals.get_leader()

    // e.g.
    // auto storages = m_externals.get_storage_services();
    // auto offset = storages.at(m_externals.get_leader()).alloc(size);
    //
    // switch (*m_externals.get_group_state()) {
    // case group_state::HEALTHY:
    //     for (int i = 0; i < group_config.data_shards; i++) {
    //         auto addr = address{offset, i};
    //         storages.at(i).write(addr, data);
    //     }
    //     break;
    // default:
    //     // do not permit
    //     break;
    // }

    co_return address{};
}

coro<shared_buffer<>> ec_data_view::read(const uint128_t& pointer,
                                         size_t size) {
    co_return shared_buffer<>{};
}

coro<std::size_t> ec_data_view::read_address(const address& addr,
                                             std::span<char> buffer) {
    co_return 0;
}

coro<std::size_t> ec_data_view::get_used_space() { co_return 0; }

[[nodiscard]] coro<address> ec_data_view::link(const address& addr) {
    co_return address{};
}

coro<std::size_t> ec_data_view::unlink(const address& addr) { co_return 0; }

} // namespace uh::cluster::storage
