#include "ec_data_view.h"
#include "impl/address_utils.h"

#include <common/telemetry/log.h>

namespace uh::cluster::storage {
ec_data_view::ec_data_view(boost::asio::io_context& ioc, etcd_manager& etcd,
                           std::size_t group_id, group_config group_config,
                           std::size_t service_connections)
    : m_ioc(ioc),
      m_storage_index{group_config.storages},
      m_storage_maintainer(
          etcd, ns::root.storage_groups[group_id].storage_hostports,
          service_factory<storage_interface>(ioc, service_connections),
          {m_storage_index}) {}

coro<address> ec_data_view::write(std::span<const char> data,
                                  const std::vector<std::size_t>& offsets) {
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
