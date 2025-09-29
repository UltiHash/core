#pragma once

#include "config.h"

#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/etcd/service_discovery/storage_index.h>
#include <common/types/scoped_buffer.h>
#include <storage/group/externals.h>
#include <storage/interfaces/data_view.h>

namespace uh::cluster::storage {

class rr_data_view : public data_view {
    friend class cache;

public:
    explicit rr_data_view(boost::asio::io_context& ioc, etcd_manager& etcd,
                          std::size_t group_id, group_config group_config,
                          std::size_t service_connections);

    coro<address> write(std::span<const char> data);

    coro<shared_buffer<>> read(const uint128_t& pointer, size_t size);

    coro<std::size_t> read_address(const address& addr, std::span<char> buffer);

    [[nodiscard]] coro<address> link(const address& addr);

    coro<std::size_t> unlink(const address& addr);

    coro<std::size_t> get_used_space();

private:
    boost::asio::io_context& m_ioc;
    group_config m_group_config;

    service_load_balancer<storage_interface> m_load_balancer;
    storage_index m_storage_index;
    service_maintainer<storage_interface> m_storage_maintainer;
    std::vector<std::vector<refcount_t>>
    extract_refcounts(const address& addr) const;
    address compute_address(const std::vector<std::size_t>& offsets,
                            const std::size_t data_size,
                            const std::size_t storage_id,
                            const std::size_t base_offset) const;
    address compute_rejected_address(
        const std::vector<std::vector<refcount_t>>& rejected_refcounts,
        const address& original_addr);
};

} // namespace uh::cluster::storage
