#pragma once

#include <common/telemetry/context.h>
#include <common/types/common_types.h>
#include <common/types/scoped_buffer.h>
#include <common/utils/common.h>

namespace uh::cluster::sn {

struct interface {
    virtual coro<address> write(context& ctx, std::span<const char>,
                                const std::vector<std::size_t>&) = 0;

    virtual coro<std::size_t> read(context& ctx, const address& addr,
                                   std::span<char>) = 0;

    virtual coro<address> link(context& ctx, const address& addr) = 0;
    virtual coro<std::size_t> unlink(context& ctx, const address& addr) = 0;
    virtual coro<std::size_t> get_used_space(context& ctx) = 0;

    virtual coro<std::map<size_t, size_t>> get_ds_size_map(context&) {
        throw std::runtime_error("not implemented");
    }

    virtual coro<void> ds_write(context&, uint32_t, uint64_t,
                                std::span<const char>) {
        throw std::runtime_error("not implemented");
    }

    virtual coro<void> ds_read(context&, uint32_t, uint64_t, std::size_t,
                               char*) {
        throw std::runtime_error("not implemented");
    }

    virtual ~interface() = default;
    static constexpr role service_role = STORAGE_SERVICE;
};

} // namespace uh::cluster::sn
