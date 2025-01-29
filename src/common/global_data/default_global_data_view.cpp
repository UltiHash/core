#include "default_global_data_view.h"

#include "common/utils/address_utils.h"

namespace uh::cluster {
default_global_data_view::default_global_data_view(
    const global_data_view_config& config, boost::asio::io_context& ioc,
    etcd_manager& etcd)
    : m_io_service(ioc),
      m_config(config),
      m_service_maintainer(
          etcd, remote_factory(m_io_service,
                               config.storage_service_connection_count)),
      m_ec_maintainer(m_io_service, m_config.ec_data_shards,
                      m_config.ec_parity_shards, etcd, false),
      m_basic_getter(m_config.ec_data_shards, m_config.ec_parity_shards) {

    m_service_maintainer.add_monitor(m_ec_maintainer);
    m_ec_maintainer.add_monitor(m_load_balancer);
    m_ec_maintainer.add_monitor(m_basic_getter);

    m_load_balancer.get();
}

coro<address>
default_global_data_view::write(context& ctx, std::span<const char> data,
                                const std::vector<std::size_t>& offsets) {
    const auto client = m_load_balancer.get();
    co_return co_await client->write(ctx, data, offsets);
}

coro<std::size_t> default_global_data_view::read(context& ctx,
                                                 const address& addr,
                                                 std::span<char> buffer) {
    co_return co_await perform_for_address(
        addr, m_basic_getter, m_io_service,
        [&ctx, buffer](size_t, std::shared_ptr<distributed_storage> dn,
                       const address_info& info) -> coro<void> {
            co_await dn->read_address(ctx, info.addr, buffer,
                                      info.pointer_offsets);
        });
}

coro<std::size_t> default_global_data_view::get_used_space(context& ctx) {
    auto nodes = m_basic_getter.get_services();

    size_t used = 0;
    for (const auto& dn : nodes) {
        used += co_await dn->get_used_space(ctx);
    }

    co_return used;
}

coro<address> default_global_data_view::link(context& ctx,
                                             const address& addr) {
    std::map<size_t, address> addresses;
    co_await perform_for_address(
        addr, m_basic_getter, m_io_service,
        [&ctx, &addresses](size_t id, std::shared_ptr<distributed_storage> dn,
                           const address_info& info) -> coro<void> {
            addresses.emplace(id, co_await dn->link(ctx, info.addr));
        });

    address rv;
    for (const auto& a : addresses) {
        rv.append(a.second);
    }

    co_return rv;
}

coro<std::size_t> default_global_data_view::unlink(context& ctx,
                                                   const address& addr) {
    std::atomic<size_t> freed_bytes;
    co_await perform_for_address(
        addr, m_basic_getter, m_io_service,
        [&ctx, &freed_bytes](size_t, std::shared_ptr<distributed_storage> dn,
                             const address_info& info) -> coro<void> {
            freed_bytes += co_await dn->unlink(ctx, info.addr);
        });
    co_return freed_bytes;
}

default_global_data_view::~default_global_data_view() noexcept {
    m_ec_maintainer.remove_monitor(m_load_balancer);
    m_ec_maintainer.remove_monitor(m_basic_getter);
    m_service_maintainer.remove_monitor(m_ec_maintainer);
}

} // namespace uh::cluster
