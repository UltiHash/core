#pragma once

#include "common/types/address.h"
#include "common/types/big_int.h"
#include "common/types/common_types.h"
#include "common/types/scoped_buffer.h"

namespace uh::cluster::storage {

class data_view {

public:
    virtual coro<address> write(std::span<const char> data,
                                const std::vector<std::size_t>& offsets) = 0;

    virtual coro<shared_buffer<>> read(const pointer& pointer, size_t size) = 0;

    virtual coro<std::size_t> read_address(const address& addr,
                                           std::span<char> buffer) = 0;

    [[nodiscard]] virtual coro<address> link(const address& addr) = 0;

    virtual coro<std::size_t> unlink(const address& addr) = 0;

    virtual coro<std::size_t> get_used_space() = 0;

    virtual ~data_view() noexcept = default;
};

} // namespace uh::cluster::storage
