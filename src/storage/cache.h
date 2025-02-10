#pragma once

#include <common/caches/lru_cache.h>
#include <common/telemetry/metrics.h>
#include <storage/interface.h>

namespace uh::cluster::sn {

class cache : public sn::interface {
public:
    cache(sn::interface& storage, std::size_t capacity);

    coro<address> write(context& ctx, std::span<const char>,
                        const std::vector<std::size_t>&) override;

    coro<std::size_t> read(context& ctx, const address& addr,
                           std::span<char> buffer) override;

    coro<shared_buffer<>> read(context& ctx, uint128_t pointer,
                               std::size_t size);

    coro<address> link(context& ctx, const address& addr) override;
    coro<std::size_t> unlink(context& ctx, const address& addr) override;
    coro<std::size_t> get_used_space(context& ctx) override;
    coro<std::map<size_t, size_t>> get_ds_size_map(context& ctx) override;

    coro<void> ds_write(context& ctx, uint32_t ds_id, uint64_t pointer,
                        std::span<const char>) override;
    coro<void> ds_read(context& ctx, uint32_t ds_id, uint64_t pointer,
                       std::size_t size, char* buffer) override;

private:
    sn::interface& m_storage;
    lru_cache<uint128_t, shared_buffer<char>> m_lru;
};

} // namespace uh::cluster::sn
