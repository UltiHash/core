#include "rr_data_view.h"
#include "impl/address_utils.h"

#include <common/telemetry/log.h>

#include <numeric>
#include <ranges>

namespace uh::cluster::storage {
rr_data_view::rr_data_view(boost::asio::io_context& ioc, etcd_manager& etcd,
                           std::size_t group_id, group_config group_config,
                           std::size_t service_connections)
    : m_ioc(ioc),
      m_group_config{group_config},
      m_load_balancer{},
      m_storage_index{group_config.storages},
      m_storage_maintainer(
          etcd, ns::root.storage_groups[group_id].storage_hostports,
          service_factory<storage_interface>(ioc, service_connections),
          {m_load_balancer, m_storage_index}) {
    m_load_balancer.get();
}

coro<address> rr_data_view::write(std::span<const char> data,
                                  const std::vector<std::size_t>& offsets) {
    const auto [storage_id, client] = m_load_balancer.get();
    auto allocation = co_await client->allocate(data.size());
    auto addr = co_await client->write(allocation, data, offsets);
    address rv;
    for (auto i = 0ul; i < addr.size(); i++) {
        auto frag = addr.get(i);
        frag.pointer = pointer_traits::rr::get_global_pointer(
            frag.pointer, m_group_config.id, storage_id);
        rv.push(frag);
    }
    co_return rv;
}

coro<shared_buffer<>> rr_data_view::read(const uint128_t& pointer,
                                         size_t size) {

    if (size == 0) {
        throw std::runtime_error("Read size must be larger than zero");
    }

    auto [id, storage_pointer] =
        pointer_traits::rr::get_storage_pointer(pointer);
    auto storage = m_storage_index.at(id);
    co_return co_await storage->read(storage_pointer, size);
}

coro<std::size_t> rr_data_view::read_address(const address& addr,
                                             std::span<char> buffer) {
    co_await perform_for_address<void>(
        m_ioc, addr, pointer_traits::rr::get_storage_pointer,
        [buffer](std::shared_ptr<storage_interface> svc,
                 const address_info& info) -> coro<void> {
            co_await svc->read_address(info.addr, buffer, info.pointer_offsets);
        },
        m_storage_index.get());

    co_return addr.data_size();
}

coro<std::size_t> rr_data_view::get_used_space() {
    auto nodes = m_storage_index.get();

    size_t used = 0;
    for (const auto& dn : nodes) {
        if (dn != nullptr) {
            used += co_await dn->get_used_space();
        }
    }
    co_return used;
}

[[nodiscard]] coro<address> rr_data_view::link(const address& addr) {
    auto addr_map = co_await perform_for_address<address>(
        m_ioc, addr, pointer_traits::rr::get_storage_pointer,
        [](std::shared_ptr<storage_interface> svc, const address_info& info)
            -> coro<address> { co_return co_await svc->link(info.addr); },
        m_storage_index.get());

    address rv;
    for (const auto& a : addr_map | std::views::values) {
        rv.append(a);
    }

    co_return rv;
}

coro<std::size_t> rr_data_view::unlink(const address& addr) {
    auto freed_bytes_map = co_await perform_for_address<std::size_t>(
        m_ioc, addr, pointer_traits::rr::get_storage_pointer,
        [](std::shared_ptr<storage_interface> svc, const address_info& info)
            -> coro<std::size_t> { co_return co_await svc->unlink(info.addr); },
        m_storage_index.get());

    auto freed_bytes = std::accumulate(
        std::ranges::begin(freed_bytes_map | std::views::values),
        std::ranges::end(freed_bytes_map | std::views::values), 0ul,
        [](std::size_t acc, std::size_t val) { return acc + val; });
    co_return freed_bytes;
}

} // namespace uh::cluster::storage
