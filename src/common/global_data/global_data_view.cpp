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
      m_ec_maintainer(m_io_service, 1, 0),
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

    auto info = extract_node_address_map(addr, m_basic_getter);

    std::vector<std::shared_ptr<awaitable_promise<void>>> promises;
    promises.reserve(info.nodes.size());

    for (auto& dn : info.nodes) {
        promises.emplace_back(
            std::make_shared<awaitable_promise<void>>(m_io_service));

        boost::asio::co_spawn(m_io_service,
                              dn->read_address(ctx, buffer,
                                               info.node_address_map[dn],
                                               info.node_data_offsets_map[dn]),
                              use_awaitable_promise_cospawn(promises.back()));
    }

    for (auto& p : promises) {
        co_await p->get();
    }

    co_return info.data_size;
}

coro<void> global_data_view::sync(context& ctx, const address& addr) {

    if (addr.empty()) [[unlikely]] {
        throw std::length_error("Empty address is not allowed for sync");
    }

    auto info = extract_node_address_map(addr, m_basic_getter);

    std::vector<std::shared_ptr<awaitable_promise<void>>> proms;
    proms.reserve(info.nodes.size());

    for (auto& dn : info.nodes) {
        proms.emplace_back(
            std::make_shared<awaitable_promise<void>>(m_io_service));
        boost::asio::co_spawn(m_io_service,
                              dn->sync(ctx, info.node_address_map[dn]),
                              use_awaitable_promise_cospawn(proms.back()));
    }

    for (auto& p : proms) {
        co_await p->get();
    }
}

coro<std::size_t> global_data_view::get_used_space(context& ctx) {
    auto nodes = m_basic_getter.get_services();

    size_t used = 0;
    for (const auto& dn : nodes) {
        used += co_await dn->get_used_space(ctx);
    }
    co_return used;
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

[[nodiscard]] coro<address> global_data_view::link(context& ctx,
                                                   const address& addr) {

    auto info = extract_node_address_map(addr, m_basic_getter);

    std::vector<std::shared_ptr<awaitable_promise<address>>> proms;
    proms.reserve(info.nodes.size());

    for (auto& dn : info.nodes) {
        proms.emplace_back(
            std::make_shared<awaitable_promise<address>>(m_io_service));
        boost::asio::co_spawn(m_io_service,
                              dn->link(ctx, info.node_address_map[dn]),
                              use_awaitable_promise_cospawn(proms.back()));
    }

    address rv;
    for (auto& p : proms) {
        rv.append(co_await p->get());
    }

    co_return rv;
}

coro<void> global_data_view::unlink(context& ctx, const address& addr) {

    auto info = extract_node_address_map(addr, m_basic_getter);

    std::vector<std::shared_ptr<awaitable_promise<void>>> proms;
    proms.reserve(info.nodes.size());

    for (auto& dn : info.nodes) {
        proms.emplace_back(
            std::make_shared<awaitable_promise<void>>(m_io_service));
        boost::asio::co_spawn(m_io_service,
                              dn->unlink(ctx, info.node_address_map[dn]),
                              use_awaitable_promise_cospawn(proms.back()));
    }

    for (auto& p : proms) {
        co_await p->get();
    }
}

} // namespace uh::cluster
