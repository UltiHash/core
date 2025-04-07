#include "default_global_data_view.h"

#include <storage/address_utils.h>

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
default_global_data_view::write(std::span<const char> data,
                                const std::vector<std::size_t>& offsets) {
    const auto client = m_load_balancer.get();
    co_return co_await client->write(data, offsets);
}

shared_buffer<char>
default_global_data_view::read_fragment(const uint128_t& pointer,
                                        const size_t size) {

    if (size == 0) {
        throw std::runtime_error("Read fragment size must be larger than zero");
    }

    shared_buffer<char> buffer(size);
    const fragment frag{pointer, size};
    auto storage = m_basic_getter.get(pointer);
    auto context = THREAD_LOCAL_CONTEXT;

    if (boost::asio::trace_span::enable &&
        !boost::asio::trace_span::check_context(context)) {
        LOG_ERROR() << "[read_fragment] The context to be "
                       "encoded is invalid";
    }

    boost::asio::co_spawn(
        m_io_service,
        storage->read_fragment(buffer.data(), frag).continue_trace(context),
        boost::asio::use_future)
        .get();
    return buffer;
}

coro<shared_buffer<>> default_global_data_view::read(const uint128_t& pointer,
                                                     size_t size) {

    if (size == 0) {
        throw std::runtime_error("Read size must be larger than zero");
    }

    auto storage = m_basic_getter.get(pointer);
    co_return co_await storage->read(pointer, size);
}

coro<std::size_t>
default_global_data_view::read_address(const address& addr,
                                       std::span<char> buffer) {
    co_return co_await perform_for_address(
        addr, m_basic_getter, m_io_service,
        [buffer](size_t, std::shared_ptr<storage_interface> dn,
                 const address_info& info) -> coro<void> {
            co_await dn->read_address(info.addr, buffer, info.pointer_offsets);
        });
}

coro<std::size_t> default_global_data_view::get_used_space() {
    auto nodes = m_basic_getter.get_services();

    size_t used = 0;
    for (const auto& dn : nodes) {
        used += co_await dn->get_used_space();
    }
    co_return used;
}

[[nodiscard]] coro<address>
default_global_data_view::link(const address& addr) {
    std::map<size_t, address> addresses;
    co_await perform_for_address(
        addr, m_basic_getter, m_io_service,
        [&addresses](size_t id, std::shared_ptr<storage_interface> dn,
                     const address_info& info) -> coro<void> {
            addresses.emplace(id, co_await dn->link(info.addr));
        });

    address rv;
    for (const auto& a : addresses) {
        rv.append(a.second);
    }

    co_return rv;
}

coro<std::size_t> default_global_data_view::unlink(const address& addr) {
    std::atomic<size_t> freed_bytes;
    co_await perform_for_address(
        addr, m_basic_getter, m_io_service,
        [&freed_bytes](size_t, std::shared_ptr<storage_interface> dn,
                       const address_info& info) -> coro<void> {
            freed_bytes += co_await dn->unlink(info.addr);
        });
    co_return freed_bytes;
}

default_global_data_view::~default_global_data_view() noexcept {
    m_ec_maintainer.remove_monitor(m_load_balancer);
    m_ec_maintainer.remove_monitor(m_basic_getter);
    m_service_maintainer.remove_monitor(m_ec_maintainer);
}

} // namespace uh::cluster
