#pragma once

#include <common/network/client.h>
#include <common/telemetry/context.h>

#include <span>
#include <vector>

namespace uh::cluster::sn {

coro<address> write(client::acquired_messenger& m, context& ctx,
                    std::span<const char> data,
                    const std::vector<std::size_t>& offsets);

coro<std::size_t> read(client::acquired_messenger& m, context& ctx,
                       const address& addr, std::span<char> buffer);

coro<std::size_t> read_address(client::acquired_messenger& m, context& ctx,
                        const address& addr, std::span<char> buffer,
                        const std::vector<size_t>& offsets);

coro<address> link(client::acquired_messenger& m, context& ctx,
                   const address& addr);

coro<std::size_t> unlink(client::acquired_messenger& m, context& ctx,
                         const address& addr);

coro<std::size_t> get_used_space(client::acquired_messenger& m, context& ctx);

coro<std::map<size_t, size_t>> get_ds_size_map(client::acquired_messenger& m,
                                               context& ctx);

coro<void> ds_write(client::acquired_messenger& m, context& ctx, uint32_t ds_id,
                    uint64_t pointer, std::span<const char> data);

coro<void> ds_read(client::acquired_messenger& m, context& ctx, uint32_t ds_id,
                   uint64_t pointer, size_t size, char* buffer);

} // namespace uh::cluster::sn
