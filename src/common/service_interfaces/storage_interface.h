#pragma once

#include "common/types/common_types.h"
#include "common/types/scoped_buffer.h"
#include "common/utils/common.h"

namespace uh::cluster {
struct storage_interface {
    virtual coro<allocation_t>
    allocate(std::size_t size, std::size_t alignment = DEFAULT_PAGE_SIZE) = 0;

    virtual coro<address>
    write(allocation_t allocation,
          const std::vector<std::span<const char>>& buffers,
          std::span<const std::size_t> offsets) = 0;

    coro<address> write(const allocation_t allocation, //
                        const std::vector<std::span<const char>>& buffers,
                        std::initializer_list<std::size_t> offsets) {
        co_return co_await write(
            allocation, buffers,
            std::span<const std::size_t>(offsets.begin(), offsets.size()));
    }

    virtual coro<shared_buffer<>> read(const uint128_t& pointer,
                                       size_t size) = 0;

    virtual coro<void> read_address(const address& addr, std::span<char> buffer,
                                    const std::vector<size_t>& offsets) = 0;

    virtual coro<address> link(const address& addr) = 0;

    virtual coro<std::size_t> unlink(const address& addr) = 0;

    // virtual coro<std::vector<refcount_t>> get_reference_counters(const
    // std::vector<std::size_t>& stripe_ids) = 0;

    // virtual coro<void> set_reference_counters(const std::vector<refcount_t>&
    // refcounts) = 0;

    virtual coro<std::size_t> get_used_space() = 0;

    virtual ~storage_interface() noexcept = default;

    static constexpr role service_role = STORAGE_SERVICE;
};

} // namespace uh::cluster
