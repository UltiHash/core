#ifndef CORE_COMMON_SERVICE_INTERFACES_STORAGE_INTERFACE_H
#define CORE_COMMON_SERVICE_INTERFACES_STORAGE_INTERFACE_H

#include "common/telemetry/context.h"
#include "common/types/common_types.h"
#include "common/types/scoped_buffer.h"
#include "common/utils/common.h"

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
    virtual coro<std::size_t> unlink(context& ctx, const address& addr) = 0;
    virtual coro<std::size_t> get_used_space(context& ctx) = 0;
    virtual coro<std::map<size_t, size_t>> get_ds_size_map(context& ctx) = 0;
    virtual coro<void> ds_write(context& ctx, uint32_t ds_id, uint64_t pointer,
                                const std::string_view&) = 0;
    virtual coro<void> ds_read(context& ctx, uint32_t ds_id, uint64_t pointer,
                               std::size_t size, char* buffer) = 0;

    virtual ~storage_interface() = default;
    static constexpr role service_role = STORAGE_SERVICE;
};

} // namespace uh::cluster

#endif
