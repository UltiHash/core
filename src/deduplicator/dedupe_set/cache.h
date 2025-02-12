#pragma once

#include <common/caches/lru_cache.h>
#include <storage/interface.h>

#include <boost/asio.hpp>

namespace uh::cluster::dd {

class cache {
public:
    struct use_coro {};

    cache(boost::asio::io_context& ioc, sn::interface& gdv,
          std::size_t capacity);

    shared_buffer<> read(context& ctx, const uint128_t& pointer, size_t size);

    coro<shared_buffer<>> read(context& ctx, const uint128_t& pointer,
                               size_t size, use_coro);

private:
    boost::asio::io_context& m_ioc;
    sn::interface& m_gdv;
    lru_cache<uint128_t, shared_buffer<char>> m_lru;
};

} // namespace uh::cluster::dd
