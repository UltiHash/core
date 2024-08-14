
#ifndef UH_CLUSTER_STORAGE_INTERFACE_H
#define UH_CLUSTER_STORAGE_INTERFACE_H

#include "common/network/messenger_core.h"
#include "common/types/address.h"

namespace uh::cluster {
struct storage_interface {
    virtual coro<address> write(context& ctx, const std::string_view&) = 0;
    virtual coro<void> read_fragment(context& ctx, char* buffer,
                                     const fragment& f) = 0;
    virtual coro<shared_buffer<>> read(context& ctx, const uint128_t& pointer,
                                       size_t size) = 0;
    virtual coro<void> read_address(context& ctx, char* buffer,
                                    const address& addr,

                                    const std::vector<size_t>& offsets) = 0;

    virtual coro<address> link(context& ctx, const address& addr) = 0;
    virtual coro<void> unlink(context& ctx, const address& addr) = 0;
    virtual coro<void> sync(context& ctx, const address& addr) = 0;
    virtual coro<size_t> get_used_space(context& ctx) = 0;

    virtual ~storage_interface() = default;
    static constexpr role service_role = STORAGE_SERVICE;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_STORAGE_INTERFACE_H
