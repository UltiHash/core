#include "default_global_data_view.h"

#include "common/utils/address_utils.h"

namespace uh::cluster {
default_global_data_view::default_global_data_view(
    const global_data_view_config& config, boost::asio::io_context& ioc,
    service_maintainer<storage_interface>& storage_maintainer,
    etcd_manager& etcd)
    : m_io_service(ioc),
      m_config(config),
      m_service_maintainer(storage_maintainer),
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

shared_buffer<char>
default_global_data_view::read_fragment(context& ctx, const uint128_t& pointer,
                                        const size_t size) {

    if (size == 0) {
        throw std::runtime_error("Read fragment size must be larger than zero");
    }

    shared_buffer<char> buffer(size);
    const fragment frag{pointer, size};
    auto storage = m_basic_getter.get(pointer);
    boost::asio::co_spawn(m_io_service,
                          storage->read_fragment(ctx, buffer.data(), frag),
                          boost::asio::use_future)
        .get();
    return buffer;
}

coro<shared_buffer<>> default_global_data_view::read(context& ctx,
                                                     const uint128_t& pointer,
                                                     size_t size) {

    if (size == 0) {
        throw std::runtime_error("Read size must be larger than zero");
    }

    auto storage = m_basic_getter.get(pointer);
    co_return co_await storage->read(ctx, pointer, size);
}

coro<std::size_t>
default_global_data_view::read_address(context& ctx, const address& addr,
                                       std::span<char> buffer) {
    co_return co_await perform_for_address(
        addr, m_basic_getter, m_io_service,
        [&ctx, buffer](size_t, std::shared_ptr<storage_interface> dn,
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

[[nodiscard]] coro<address>
default_global_data_view::link(context& ctx, const address& addr) {
    std::map<size_t, address> addresses;
    co_await perform_for_address(
        addr, m_basic_getter, m_io_service,
        [&ctx, &addresses](size_t id, std::shared_ptr<storage_interface> dn,
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
        [&ctx, &freed_bytes](size_t, std::shared_ptr<storage_interface> dn,
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
