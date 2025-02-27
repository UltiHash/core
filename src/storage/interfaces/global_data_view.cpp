#include "global_data_view.h"

#include <storage/address_utils.h>
#include <storage/protocol.h>

namespace uh::cluster {

client_factory::client_factory(boost::asio::io_context& ioc,
                               unsigned connections)
    : m_ioc(ioc),
      m_connections(connections) {}

std::shared_ptr<client>
client_factory::make_service(const std::string& hostname, uint16_t port, int) {
    return std::make_shared<client>(m_ioc, hostname, port, m_connections);
}

global_data_view::global_data_view(
    boost::asio::io_context& ioc,
    service_maintainer<client, client_factory, STORAGE_SERVICE>&
        storage_maintainer)
    : m_io_service(ioc),
      m_service_maintainer(storage_maintainer) {
    m_service_maintainer.add_monitor(m_getter);
}

global_data_view::~global_data_view() {
    m_service_maintainer.remove_monitor(m_getter);
}

coro<address>
global_data_view::write(context& ctx, std::span<const char> data,
                                const std::vector<std::size_t>& offsets) {

    auto services = m_getter.get_services();
    if (services.empty()) {
        throw std::runtime_error("no storage services available");
    }

    auto client = services.at(m_round_robin % services.size());
    ++m_round_robin;

    auto m = co_await client->acquire_messenger();

    co_return co_await sn::write(m, ctx, data, offsets);
}

coro<std::size_t> global_data_view::read(context& ctx,
                                                 const address& addr,
                                                 std::span<char> buffer) {

    std::atomic<std::size_t> read_bytes = 0;

    co_await perform_for_address(
        addr, m_getter, m_io_service,
        [&ctx, buffer, &read_bytes](size_t, std::shared_ptr<client> dn,
                                    const address_info& info) -> coro<void> {
            auto m = co_await dn->acquire_messenger();
            read_bytes += co_await sn::read_address(m, ctx, info.addr, buffer,
                                                    info.pointer_offsets);
        });

    co_return read_bytes;
}

coro<std::size_t> global_data_view::get_used_space(context& ctx) {
    auto nodes = m_getter.get_services();

    size_t used = 0;
    for (const auto& dn : nodes) {
        auto m = co_await dn->acquire_messenger();
        used += co_await sn::get_used_space(m, ctx);
    }
    co_return used;
}

[[nodiscard]] coro<address>
global_data_view::link(context& ctx, const address& addr) {
    std::map<size_t, address> addresses;
    co_await perform_for_address(
        addr, m_getter, m_io_service,
        [&ctx, &addresses](size_t id, std::shared_ptr<client> dn,
                           const address_info& info) -> coro<void> {
            auto m = co_await dn->acquire_messenger();
            auto addr = co_await sn::link(m, ctx, info.addr);
            addresses.emplace(id, addr);
        });

    address rv;
    for (const auto& a : addresses) {
        rv.append(a.second);
    }

    co_return rv;
}

coro<std::size_t> global_data_view::unlink(context& ctx,
                                                   const address& addr) {
    std::atomic<size_t> freed_bytes;
    co_await perform_for_address(
        addr, m_getter, m_io_service,
        [&ctx, &freed_bytes](size_t, std::shared_ptr<client> dn,
                             const address_info& info) -> coro<void> {
            auto m = co_await dn->acquire_messenger();
            freed_bytes += co_await sn::unlink(m, ctx, info.addr);
        });
    co_return freed_bytes;
}

} // namespace uh::cluster
