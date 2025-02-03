#pragma once

#include "common/telemetry/context.h"
#include "common/types/common_types.h"
#include "common/types/scoped_buffer.h"
#include "common/utils/common.h"

namespace uh::cluster {
struct storage_interface {
    /**
     * Write `buffer` to the storage, return the address it was stored at.
     *
     * @param ctx traces context
     * @param buffer data to write
     * @param offsets
     * @return address of written data
     */
    virtual coro<address> write(context& ctx, std::span<const char> buffer,
                                const std::vector<std::size_t>& offsets) = 0;

    /**
     * Read data specified by `addr`. Return amount of data read.
     *
     * @param ctx traces context
     * @param[out] buffer A char buffer that the retrieved data is written to.
     * @param[in] addr address to read
     * @return The number of bytes read.
     */
    virtual coro<std::size_t> read(context& ctx, const address& addr,
                                   std::span<char> buffer) = 0;

    /**
     * Increment the reference counter for all entries in the address. Return
     * the portion of the address that could not be referenced as it was already
     * freed.
     *
     * @param ctx traces context
     * @param addr The address specifying the storage regions to be referenced.
     * @return An address specifying all storage regions within #addr that have
     * already been deleted and therefore can no longer be referenced.
     */
    virtual coro<address> link(context& ctx, const address& addr) = 0;

    /**
     * Decrement the reference counter for all entries in the address. Return
     * the amount of data freed.
     *
     * @param ctx traces context
     * @param addr address to de-reference
     * @return number of bytes freed
     */
    virtual coro<std::size_t> unlink(context& ctx, const address& addr) = 0;

    /**
     * @brief Return storage space used by this storage
     * @param ctx traces context
     * @return used space
     */
    virtual coro<std::size_t> get_used_space(context& ctx) = 0;

    virtual coro<std::map<size_t, size_t>> get_ds_size_map(context& ctx) {
        throw std::runtime_error("not implemented");
    }

    virtual coro<void> ds_write(context& ctx, uint32_t ds_id, uint64_t pointer,
                                std::span<const char>) {
        throw std::runtime_error("not implemented");
    }

    virtual coro<void> ds_read(context& ctx, uint32_t ds_id, uint64_t pointer,
                               std::size_t size, char* buffer) {
        throw std::runtime_error("not implemented");
    }

    virtual ~storage_interface() = default;
    static constexpr role service_role = STORAGE_SERVICE;
};

} // namespace uh::cluster
