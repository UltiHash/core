#include "data_view.h"

#include <common/telemetry/log.h>
#include <storage/group/config.h>
#include <storage/group/ec_data_view.h>
#include <storage/group/rr_data_view.h>

namespace uh::cluster::storage::global {

std::unique_ptr<data_view> group_factory(boost::asio::io_context& ioc,
                                         etcd_manager& etcd,
                                         std::size_t group_id,
                                         const storage::group_config& config,
                                         std::size_t service_connections) {
    switch (config.type) {
    case group_config::type_t::ROUND_ROBIN:
        return std::make_unique<rr_data_view>(
            ioc, etcd, group_id, std::move(config), service_connections);
    case group_config::type_t::ERASURE_CODING:
        return std::make_unique<ec_data_view>(
            ioc, etcd, group_id, std::move(config), service_connections);
    case group_config::type_t::REPLICA:
    default:
        throw std::runtime_error("Unsupported group type");
    }
}

global_data_view::global_data_view(boost::asio::io_context& ioc,
                                   etcd_manager& etcd,
                                   const global_data_view_config& config)
    : m_ioc(ioc) {

    // TODO: watch group configs and create group using factory function.
    // We should save groups using map, or using two vectors like group_indices,
    // group_views.
    etcd.wait(ns::root.storage_groups.group_configs,
              time_settings::instance().get_group_state_wait_timeout());
    auto map = etcd.ls(ns::root.storage_groups.group_configs);
    if (map.size() != 1) {
        throw std::runtime_error("Now we support single storage group only");
    }

    auto key = std::filesystem::path(map.begin()->first);
    auto group_id = stoul(key.filename().string());

    auto group_config = deserialize<storage::group_config>(
        etcd.get(ns::root.storage_groups.group_configs[group_id]));

    m_group_view = group_factory(m_ioc, etcd, group_id, group_config,
                                 config.storage_service_connection_count);
}

coro<address> global_data_view::write(std::span<const char> data,
                                      const std::vector<std::size_t>& offsets) {
    co_return co_await m_group_view->write(data, offsets);
}

coro<shared_buffer<>> global_data_view::read(const uint128_t& pointer,
                                             size_t size) {

    co_return co_await m_group_view->read(pointer, size);
}

coro<std::size_t> global_data_view::read_address(const address& addr,
                                                 std::span<char> buffer) {
    co_return co_await m_group_view->read_address(addr, buffer);
}

coro<std::size_t> global_data_view::get_used_space() {
    co_return co_await m_group_view->get_used_space();
}

[[nodiscard]] coro<address> global_data_view::link(const address& addr) {
    co_return co_await m_group_view->link(addr);
}

coro<std::size_t> global_data_view::unlink(const address& addr) {
    co_return co_await m_group_view->unlink(addr);
}

} // namespace uh::cluster::storage::global
