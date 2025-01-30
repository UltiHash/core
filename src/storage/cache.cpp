#include "cache.h"

namespace uh::cluster::sn {

cache::cache(storage_interface& storage, std::size_t capacity)
    : m_storage(storage),
      m_lru(capacity) {}

coro<address> cache::write(context& ctx, std::span<const char> buffer,
                           const std::vector<std::size_t>& offsets) {
    return m_storage.write(ctx, buffer, offsets);
}

coro<std::size_t> cache::read(context& ctx, const address& addr,
                              std::span<char> buffer) {
    return m_storage.read(ctx, addr, buffer);
}

coro<shared_buffer<>> cache::read(context& ctx, uint128_t pointer,
                                  std::size_t size) {

    if (const auto cp = m_lru.get(pointer); cp && cp->size() >= size) {
        metric<metric_type::gdv_l2_cache_hit_counter>::increase(1);
        co_return cp.value();
    }

    shared_buffer<char> buffer(size);
    co_await m_storage.read(ctx, fragment{pointer, size}, buffer.span());
    m_lru.put(pointer, buffer);
    co_return buffer;
}

coro<address> cache::link(context& ctx, const address& addr) {
    return m_storage.link(ctx, addr);
}

coro<std::size_t> cache::unlink(context& ctx, const address& addr) {
    return m_storage.unlink(ctx, addr);
}

coro<std::size_t> cache::get_used_space(context& ctx) {
    return m_storage.get_used_space(ctx);
}

coro<std::map<size_t, size_t>> cache::get_ds_size_map(context& ctx) {
    return m_storage.get_ds_size_map(ctx);
}

coro<void> cache::ds_write(context& ctx, uint32_t ds_id, uint64_t pointer,
                           std::span<const char> buffer) {
    return m_storage.ds_write(ctx, ds_id, pointer, buffer);
}

coro<void> cache::ds_read(context& ctx, uint32_t ds_id, uint64_t pointer,
                          std::size_t size, char* buffer) {
    return m_storage.ds_read(ctx, ds_id, pointer, size, buffer);
}

} // namespace uh::cluster::sn
