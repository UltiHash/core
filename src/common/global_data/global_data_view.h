#pragma once

#include "common/telemetry/context.h"
#include "common/types/address.h"
#include "common/types/big_int.h"
#include "common/types/common_types.h"
#include "common/types/scoped_buffer.h"

namespace uh::cluster {

class global_data_view {

public:
    virtual coro<address> write(context& ctx, std::span<const char> data,
                                const std::vector<std::size_t>& offsets) = 0;

    virtual coro<shared_buffer<>> read(context& ctx, const uint128_t& pointer,
                                       size_t size) = 0;
    virtual shared_buffer<char>
    read_fragment(context& ctx, const uint128_t& pointer, size_t size) = 0;

    virtual coro<std::size_t> read_address(context& ctx, const address& addr,
                                           std::span<char> buffer) = 0;

    [[nodiscard]] virtual coro<address> link(context& ctx,
                                             const address& addr) = 0;

    virtual coro<std::size_t> unlink(context& ctx, const address& addr) = 0;

    virtual coro<std::size_t> get_used_space(context& ctx) = 0;

    virtual ~global_data_view() noexcept = default;
};

} // end namespace uh::cluster
