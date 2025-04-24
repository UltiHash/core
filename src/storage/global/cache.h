#pragma once

#include <common/caches/lru_cache.h>
#include <storage/interfaces/data_view.h>

namespace uh::cluster::storage::global {

class cache {
public:
    cache(boost::asio::io_context& ioc,
          uh::cluster::storage::data_view& storage, std::size_t capacity);

    shared_buffer<> read_fragment(const uint128_t& pointer, size_t size);

    coro<shared_buffer<>> read(const uint128_t& pointer, size_t size);

private:
    boost::asio::io_context& m_ioc;
    uh::cluster::storage::data_view& m_storage;
    lru_cache<uint128_t, shared_buffer<char>> m_lru;
};

} // namespace uh::cluster::storage::global
