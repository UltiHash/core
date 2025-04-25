#include "data_view.h"
#include "address_utils.h"

#include <common/telemetry/log.h>

namespace uh::cluster::storage::global {
global_data_view::global_data_view(
    const global_data_view_config& config, boost::asio::io_context& ioc,
    service_load_balancer<storage_interface>& load_balancer,
    storage_index& storage_index)
    : m_io_service(ioc),
      m_config(config),
      m_load_balancer{load_balancer},
      m_storage_index{storage_index} {
    m_load_balancer.get();
}

coro<address> global_data_view::write(std::span<const char> data,
                                      const std::vector<std::size_t>& offsets) {
    const auto client = m_load_balancer.get();
    co_return co_await client->write(data, offsets);
}

coro<shared_buffer<>> global_data_view::read(const uint128_t& pointer,
                                             size_t size) {

    if (size == 0) {
        throw std::runtime_error("Read size must be larger than zero");
    }

    auto storage = m_storage_index.get(pointer);
    co_return co_await storage->read(pointer, size);
}

coro<std::size_t> global_data_view::read_address(const address& addr,
                                                 std::span<char> buffer) {
    co_return co_await perform_for_address(
        addr, m_storage_index, m_io_service,
        [buffer](size_t, std::shared_ptr<storage_interface> svc,
                 const address_info& info) -> coro<void> {
            co_await svc->read_address(info.addr, buffer, info.pointer_offsets);
        });
}

coro<std::size_t> global_data_view::get_used_space() {
    auto nodes = m_storage_index.get_services();

    size_t used = 0;
    for (const auto& dn : nodes) {
        used += co_await dn->get_used_space();
    }
    co_return used;
}

[[nodiscard]] coro<address> global_data_view::link(const address& addr) {
    std::map<size_t, address> addresses;
    co_await perform_for_address(
        addr, m_storage_index, m_io_service,
        [&addresses](size_t id, std::shared_ptr<storage_interface> svc,
                     const address_info& info) -> coro<void> {
            addresses.emplace(id, co_await svc->link(info.addr));
        });

    address rv;
    for (const auto& a : addresses) {
        rv.append(a.second);
    }

    co_return rv;
}

coro<std::size_t> global_data_view::unlink(const address& addr) {
    std::atomic<size_t> freed_bytes;
    co_await perform_for_address(
        addr, m_storage_index, m_io_service,
        [&freed_bytes](size_t, std::shared_ptr<storage_interface> svc,
                       const address_info& info) -> coro<void> {
            freed_bytes += co_await svc->unlink(info.addr);
        });
    co_return freed_bytes;
}

} // namespace uh::cluster::storage::global
