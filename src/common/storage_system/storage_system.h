//
// Created by massi on 7/31/24.
//

#ifndef STORAGE_SYSTEM_H
#define STORAGE_SYSTEM_H
#include "common/registry/service_basic_getter.h"
#include "common/service_interfaces/storage_interface.h"

namespace uh::cluster {

struct storage_system_config {};

template <> class ec_groups : service_maintainer<storage_interface> {
    using service_maintainer<storage_interface>::service_maintainer;
    using service_maintainer<storage_interface>::m_mutex;
    using service_maintainer<storage_interface>::m_clients;
    using service_maintainer<storage_interface>::m_cv;
};

class storage_system : public storage_interface {
    explicit storage_system(const storage_system_config& config,
                            services<storage_interface>& storage_services)
        : m_storage_services(storage_services) {}

    virtual coro<address> write(context& ctx, const std::string_view&) = 0;
    virtual coro<void> read_fragment(context& ctx, char* buffer,
                                     const fragment& f) = 0;
    virtual coro<shared_buffer<>> read(context& ctx, const uint128_t& pointer,
                                       size_t size) = 0;
    virtual coro<void> read_address(context& ctx, char* buffer,
                                    const address& addr,

                                    const std::vector<size_t>& offsets) = 0;

    virtual coro<void> link(context& ctx, const address& addr) = 0;
    virtual coro<void> unlink(context& ctx, const address& addr) = 0;
    virtual coro<void> sync(context& ctx, const address& addr) = 0;
    virtual coro<size_t> get_used_space(context& ctx) = 0;

    std::vector<storage_interface> m_data_nodes;
    std::vector<storage_interface> m_ec_nodes;
    services<storage_interface>& m_storage_services;
};

} // namespace uh::cluster

#endif // STORAGE_SYSTEM_H
