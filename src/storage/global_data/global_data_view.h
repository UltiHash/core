#pragma once

#include "common/types/address.h"
#include "common/types/big_int.h"
#include "common/types/common_types.h"
#include "common/types/scoped_buffer.h"

namespace uh::cluster {

class global_data_view {

public:
    virtual coro<address> write(std::span<const char> data,
                                const std::vector<std::size_t>& offsets) = 0;

    virtual coro<shared_buffer<>> read(const uint128_t& pointer,
                                       size_t size) = 0;
    virtual shared_buffer<char> read_fragment(const uint128_t& pointer,
                                              size_t size) = 0;

    virtual coro<std::size_t> read_address(const address& addr,
                                           std::span<char> buffer) = 0;

    [[nodiscard]] virtual coro<address> link(const address& addr) = 0;

    virtual coro<std::size_t> unlink(const address& addr) = 0;

    virtual coro<std::size_t> get_used_space() = 0;

    virtual ~global_data_view() noexcept = default;
};

} // end namespace uh::cluster
