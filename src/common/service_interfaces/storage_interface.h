#pragma once

#include "common/types/common_types.h"
#include "common/types/scoped_buffer.h"
#include "common/utils/common.h"

namespace uh::cluster {
struct storage_interface {
    virtual coro<address> write(std::span<const char>,
                                const std::vector<std::size_t>&) = 0;

    virtual coro<void> read_fragment(char* buffer, const fragment& f) = 0;

    virtual coro<shared_buffer<>> read(const uint128_t& pointer,
                                       size_t size) = 0;

    virtual coro<void> read_address(const address& addr, std::span<char> buffer,
                                    const std::vector<size_t>& offsets) = 0;

    virtual coro<address> link(const address& addr) = 0;
    virtual coro<std::size_t> unlink(const address& addr) = 0;
    virtual coro<std::size_t> get_used_space() = 0;
    virtual coro<std::map<size_t, size_t>> get_ds_size_map() = 0;
    virtual coro<void> ds_write(uint32_t ds_id, uint64_t pointer,
                                std::span<const char>) = 0;
    virtual coro<void> ds_read(uint32_t ds_id, uint64_t pointer,
                               std::size_t size, char* buffer) = 0;

    virtual ~storage_interface() = default;
    static constexpr role service_role = STORAGE_SERVICE;
};

} // namespace uh::cluster
