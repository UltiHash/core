#include "global_data_view.h"

namespace uh::cluster {
global_data_view::global_data_view(
    const global_data_view_config& config, boost::asio::io_context& ioc,
    services<storage_interface>& storage_services)
    : m_io_service(ioc),
      m_storage_services(storage_services),
      m_config(config),
      m_cache_l2(m_config.read_cache_capacity_l2) {
    m_storage_services.get();
}

address global_data_view::write(const std::string_view& data) {
    const auto client = m_storage_services.get();
    return boost::asio::co_spawn(m_io_service, client->write(data),
                                 boost::asio::use_future)
        .get();
}

shared_buffer<char> global_data_view::read_fragment(const uint128_t& pointer,
                                                    const size_t size) {

    if (size == 0) {
        throw std::runtime_error("Read fragment size must be larger than zero");
    }
    if (const auto c = m_cache_l2.get(pointer); c.has_value()) {
        if (c->size() >= size) [[likely]] {
            metric<metric_type::gdv_l2_cache_hit_counter>::increase(1);
            return c.value();
        }
    }
    metric<metric_type::gdv_l2_cache_miss_counter>::increase(1);

    shared_buffer<char> buffer(size);
    const fragment frag{pointer, size};
    auto storage = m_storage_services.get(pointer);
    boost::asio::co_spawn(m_io_service,
                          storage->read_fragment(buffer.data(), frag),
                          boost::asio::use_future)
        .get();
    m_cache_l2.put(pointer, buffer);

    return buffer;
}

std::size_t global_data_view::read_address(char* buffer, const address& addr) {

    std::unordered_map<std::shared_ptr<storage_interface>, address>
        node_address_map;
    std::unordered_map<std::shared_ptr<storage_interface>, std::vector<size_t>>
        node_data_offsets_map;
    std::vector<std::shared_ptr<storage_interface>> nodes;

    size_t offset = 0;
    for (size_t i = 0; i < addr.size(); ++i) {

        const auto frag = addr.get_fragment(i);
        auto n = m_storage_services.get(frag.pointer);
        auto& node_address = node_address_map[n];
        if (node_address.empty()) {
            nodes.emplace_back(n);
        }
        node_address.push_fragment(frag);
        node_data_offsets_map[n].emplace_back(offset);
        offset += frag.size;
    }

    std::vector<std::future<void>> futures;
    futures.reserve(nodes.size());

    for (auto& dn : nodes) {
        futures.emplace_back(
            boost::asio::co_spawn(m_io_service,
                                  dn->read_address(buffer, node_address_map[dn],
                                                   node_data_offsets_map[dn]),
                                  boost::asio::use_future));
    }

    for (auto& f : futures) {
        f.get();
    }
    return offset;
}

void global_data_view::sync(const address& addr) {

    if (addr.empty()) [[unlikely]] {
        throw std::length_error("Empty address is not allowed for sync");
    }

    std::unordered_map<std::shared_ptr<storage_interface>, address>
        node_address_map;
    std::vector<std::shared_ptr<storage_interface>> nodes;

    for (size_t i = 0; i < addr.size(); ++i) {
        const auto frag = addr.get_fragment(i);
        auto n = m_storage_services.get(frag.pointer);
        auto& node_address = node_address_map[n];
        if (node_address.empty()) {
            nodes.emplace_back(std::move(n));
        }
        node_address.push_fragment(frag);
    }

    std::vector<std::future<void>> futures;
    futures.reserve(nodes.size());

    for (auto& dn : nodes) {
        futures.emplace_back(
            boost::asio::co_spawn(m_io_service, dn->sync(node_address_map[dn]),
                                  boost::asio::use_future));
    }

    for (auto& f : futures) {
        f.get();
    }
}

[[nodiscard]] uint128_t global_data_view::get_used_space() {
    auto nodes = m_storage_services.get_services();

    std::vector<std::future<size_t>> futures;
    futures.reserve(nodes.size());
    for (auto& dn : nodes) {
        futures.emplace_back(boost::asio::co_spawn(
            m_io_service, dn->get_used_space(), boost::asio::use_future));
    }

    uint128_t used = 0;
    for (auto& f : futures) {
        used += f.get();
    }
    return used;
}

[[nodiscard]] uint128_t global_data_view::get_available_space() {
    auto nodes = m_storage_services.get_services();

    std::vector<std::future<size_t>> futures;
    futures.reserve(nodes.size());
    for (auto& dn : nodes) {
        futures.emplace_back(boost::asio::co_spawn(
            m_io_service, dn->get_free_space(), boost::asio::use_future));
    }

    uint128_t available = 0;
    for (auto& f : futures) {
        available += f.get();
    }
    return available;
}

[[nodiscard]] boost::asio::io_context& global_data_view::get_executor() const {
    return m_io_service;
}

[[nodiscard]] std::size_t
global_data_view::get_storage_service_connection_count() const noexcept {
    return m_config.storage_service_connection_count;
}

} // namespace uh::cluster
