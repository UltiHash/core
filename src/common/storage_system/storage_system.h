//
// Created by massi on 7/31/24.
//

#ifndef STORAGE_SYSTEM_H
#define STORAGE_SYSTEM_H
#include "common/etcd/service_discovery/ec_group.h"
#include "common/service_interfaces/storage_interface.h"

namespace uh::cluster {

struct storage_system_config {};

class storage_system : public storage_interface {
    explicit storage_system()
        : m_ec_group(1, 1, 1) {}

    coro<address> write(context& ctx, const std::string_view&) {}
    coro<void> read_fragment(context& ctx, char* buffer, const fragment& f) {}
    coro<shared_buffer<>> read(context& ctx, const uint128_t& pointer,
                               size_t size) {}
    coro<void> read_address(context& ctx, char* buffer, const address& addr,

                            const std::vector<size_t>& offsets) {}

    coro<void> link(context& ctx, const address& addr) {}
    coro<void> unlink(context& ctx, const address& addr) {}
    coro<void> sync(context& ctx, const address& addr) {}
    coro<size_t> get_used_space(context& ctx) {}

    ec_group m_ec_group;
};

} // namespace uh::cluster

#endif // STORAGE_SYSTEM_H
