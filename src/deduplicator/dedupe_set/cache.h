#pragma once

#include <common/caches/lru_cache.h>
#include <common/global_data/global_data_view.h>

namespace uh::cluster::dd {

class cache {
public:
    struct use_coro {};

    cache(global_data_view& gdv, std::size_t capacity);

    shared_buffer<> read(context& ctx, const uint128_t& pointer, size_t size);

    coro<shared_buffer<>> read(context& ctx, const uint128_t& pointer,
                               size_t size, use_coro);

private:
    global_data_view& m_gdv;
    lru_cache<uint128_t, shared_buffer<char>> m_lru;
};

} // namespace uh::cluster::dd
