#include "concrete_global_data_view.h"

#include "common/utils/address_utils.h"

namespace uh::cluster {
concrete_global_data_view::concrete_global_data_view(
    const global_data_view_config& config, boost::asio::io_context& ioc,
    service_maintainer<storage_interface>& storage_maintainer)
    : m_io_service(ioc),
      m_config(config),
      m_cache_l2(m_config.read_cache_capacity_l2),
      m_service_maintainer(storage_maintainer),
      m_ec_maintainer(m_io_service, m_config.ec_data_shards,
                      m_config.ec_parity_shards,
                      m_service_maintainer.get_etcd_client(), false),
      m_basic_getter(m_config.ec_data_shards, m_config.ec_parity_shards) {

    m_service_maintainer.add_monitor(m_ec_maintainer);
    m_ec_maintainer.add_monitor(m_load_balancer);
    m_ec_maintainer.add_monitor(m_basic_getter);

    m_load_balancer.get();
}

coro<address>
concrete_global_data_view::write(context& ctx, const std::string_view& data,
                                 const std::vector<std::size_t>& offsets) {
    const auto client = m_load_balancer.get();
    co_return co_await client->write(ctx, data, offsets);
}

shared_buffer<char>
concrete_global_data_view::read_fragment(context& ctx, const uint128_t& pointer,
                                         const size_t size) {

    if (size == 0) {
        throw std::runtime_error("Read fragment size must be larger than zero");
    }
    if (const auto cp = m_cache_l2.get(pointer); cp.has_value()) {
        if (cp->size() >= size) [[likely]] {
            metric<metric_type::gdv_l2_cache_hit_counter>::increase(1);
            return cp.value();
        }
    }

    metric<metric_type::gdv_l2_cache_miss_counter>::increase(1);

    shared_buffer<char> buffer(size);
    const fragment frag{pointer, size};
    auto storage = m_basic_getter.get(pointer);
    boost::asio::co_spawn(m_io_service,
                          storage->read_fragment(ctx, buffer.data(), frag),
                          boost::asio::use_future)
        .get();
    m_cache_l2.put(pointer, buffer);
    return buffer;
}

coro<shared_buffer<>> concrete_global_data_view::read(context& ctx,
                                                      const uint128_t& pointer,
                                                      size_t size) {

    if (size == 0) {
        throw std::runtime_error("Read size must be larger than zero");
    }

    if (const auto cp = m_cache_l2.get(pointer); cp.has_value()) {
        if (cp->size() >= size) [[likely]] {
            metric<metric_type::gdv_l2_cache_hit_counter>::increase(1);
            co_return cp.value();
        }
    }

    metric<metric_type::gdv_l2_cache_miss_counter>::increase(1);

    auto storage = m_basic_getter.get(pointer);
    auto buffer = co_await storage->read(ctx, pointer, size);
    m_cache_l2.put(pointer, buffer);
    co_return buffer;
}

coro<std::size_t> concrete_global_data_view::read_address(context& ctx,
                                                          char* buffer,
                                                          const address& addr) {
    co_return co_await perform_for_address(
        addr, m_basic_getter, m_io_service,
        [&ctx, &buffer](size_t, std::shared_ptr<storage_interface> dn,
                        const address_info& info) -> coro<void> {
            co_await dn->read_address(ctx, buffer, info.addr,
                                      info.pointer_offsets);
        });
}

coro<std::size_t> concrete_global_data_view::get_used_space(context& ctx) {
    auto nodes = m_basic_getter.get_services();

    size_t used = 0;
    for (const auto& dn : nodes) {
        used += co_await dn->get_used_space(ctx);
    }
    co_return used;
}

[[nodiscard]] coro<address>
concrete_global_data_view::link(context& ctx, const address& addr) {
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

coro<std::size_t> concrete_global_data_view::unlink(context& ctx,
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

[[nodiscard]] boost::asio::io_context&
concrete_global_data_view::get_executor() const {
    return m_io_service;
}

[[nodiscard]] std::size_t
concrete_global_data_view::get_storage_service_connection_count()
    const noexcept {
    return m_config.storage_service_connection_count;
}
concrete_global_data_view::~concrete_global_data_view() noexcept {
    m_ec_maintainer.remove_monitor(m_load_balancer);
    m_ec_maintainer.remove_monitor(m_basic_getter);
    m_service_maintainer.remove_monitor(m_ec_maintainer);
}

} // namespace uh::cluster
