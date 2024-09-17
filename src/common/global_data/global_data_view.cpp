#include "global_data_view.h"

#include "common/utils/address_utils.h"

namespace uh::cluster {
global_data_view::global_data_view(
    const global_data_view_config& config, boost::asio::io_context& ioc,
    service_maintainer<storage_interface>& storage_maintainer)
    : m_io_service(ioc),
      m_config(config),
      m_cache_l2(m_config.read_cache_capacity_l2),
      m_service_maintainer(storage_maintainer),
      m_ec_maintainer(m_io_service, 1, 0,
                      m_service_maintainer.get_etcd_client(), false),
      m_basic_getter(1, 0) {

    m_service_maintainer.add_monitor(m_ec_maintainer);
    m_ec_maintainer.add_monitor(m_load_balancer);
    m_ec_maintainer.add_monitor(m_basic_getter);

    m_load_balancer.get();
}

coro<address> global_data_view::write(context& ctx,
                                      const std::string_view& data) {
    const auto client = m_load_balancer.get();
    co_return co_await client->write(ctx, data);
}

shared_buffer<char> global_data_view::read_fragment(context& ctx,
                                                    const uint128_t& pointer,
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

coro<shared_buffer<>>
global_data_view::read(context& ctx, const uint128_t& pointer, size_t size) {

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

coro<std::size_t> global_data_view::read_address(context& ctx, char* buffer,
                                                 const address& addr) {
    co_return co_await perform_for_address(
        addr, m_basic_getter, m_io_service,
        [&ctx, &buffer](auto, auto dn, const auto& info) -> coro<void> {
            co_await dn->read_address(ctx, buffer, info.addr,
                                      info.pointer_offsets);
        });
}

coro<void> global_data_view::sync(context& ctx, const address& addr) {

    if (addr.empty()) [[unlikely]] {
        throw std::length_error("Empty address is not allowed for sync");
    }

    co_await perform_for_address(
        addr, m_basic_getter, m_io_service,
        [&ctx](auto, auto dn, const auto& info) -> coro<void> {
            co_await dn->sync(ctx, info.addr);
        });
}

coro<std::size_t> global_data_view::get_used_space(context& ctx) {
    auto nodes = m_basic_getter.get_services();

    size_t used = 0;
    for (const auto& dn : nodes) {
        used += co_await dn->get_used_space(ctx);
    }
    co_return used;
}

[[nodiscard]] coro<address> global_data_view::link(context& ctx,
                                                   const address& addr) {
    std::map<size_t, address> addresses;
    co_await perform_for_address(
        addr, m_basic_getter, m_io_service,
        [&ctx, &addresses](auto id, auto dn, const auto& info) -> coro<void> {
            addresses.emplace(id, co_await dn->link(ctx, info.addr));
        });

    address rv;
    for (const auto& a : addresses) {
        rv.append(a.second);
    }

    co_return rv;
}

coro<void> global_data_view::unlink(context& ctx, const address& addr) {

    co_await perform_for_address(
        addr, m_basic_getter, m_io_service,
        [&ctx](auto, auto dn, const auto& info) -> coro<void> {
            co_await dn->unlink(ctx, info.addr);
        });
}

[[nodiscard]] boost::asio::io_context& global_data_view::get_executor() const {
    return m_io_service;
}

[[nodiscard]] std::size_t
global_data_view::get_storage_service_connection_count() const noexcept {
    return m_config.storage_service_connection_count;
}
global_data_view::~global_data_view() noexcept {
    m_ec_maintainer.remove_monitor(m_load_balancer);
    m_ec_maintainer.remove_monitor(m_basic_getter);
    m_service_maintainer.remove_monitor(m_ec_maintainer);
}

} // namespace uh::cluster
