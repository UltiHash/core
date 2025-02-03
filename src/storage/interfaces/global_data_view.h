#pragma once

#include "common/etcd/ec_groups/ec_get_handler.h"
#include "common/etcd/ec_groups/ec_group_maintainer.h"
#include "common/etcd/ec_groups/ec_load_balancer.h"
#include "common/etcd/service_discovery/service_maintainer.h"
#include "common/service_interfaces/storage_interface.h"
#include "storage/interfaces/remote_storage.h"

namespace uh::cluster {

struct global_data_view_config {
    std::size_t storage_service_connection_count = 16;
    std::size_t read_cache_capacity_l2 = 400000ul;
    std::size_t ec_data_shards = 1;
    std::size_t ec_parity_shards = 0;
};

class global_data_view : public storage_interface {

public:
    global_data_view(const global_data_view_config& config,
                     boost::asio::io_context& ioc, etcd_manager& etcd);

    coro<address> write(context& ctx, std::span<const char> data,
                        const std::vector<std::size_t>& offsets) override;

    coro<std::size_t> read(context& ctx, const address& addr,
                           std::span<char> buffer) override;

    coro<address> link(context& ctx, const address& addr) override;

    coro<std::size_t> unlink(context& ctx, const address& addr) override;

    coro<std::size_t> get_used_space(context& ctx) override;

    /**
     * Return number of services known to this GDV
     */
    std::size_t services() const;

    ~global_data_view() noexcept;

private:
    boost::asio::io_context& m_io_service;
    global_data_view_config m_config;

    service_maintainer<distributed_storage, remote_factory>
        m_service_maintainer;
    ec_group_maintainer m_ec_maintainer;
    ec_load_balancer m_load_balancer;
    ec_get_handler m_basic_getter;
};

} // end namespace uh::cluster
