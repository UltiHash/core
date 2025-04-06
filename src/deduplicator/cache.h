#pragma once

#include <common/caches/lru_cache.h>
#include <storage/global_data/global_data_view.h>

namespace uh::cluster::dd {

class cache {
public:
    cache(global_data_view& gdv, std::size_t capacity);

    shared_buffer<> read_fragment(const uint128_t& pointer, size_t size);

    coro<shared_buffer<>> read(const uint128_t& pointer, size_t size);

private:
    global_data_view& m_gdv;
    lru_cache<uint128_t, shared_buffer<char>> m_lru;
};

} // namespace uh::cluster::dd
