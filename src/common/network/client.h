#pragma once

#include "common/utils/common.h"
#include "common/utils/pool.h"
#include "messenger.h"
#include "tools.h"
#include <boost/asio.hpp>

namespace uh::cluster {

class client {
public:
    using acquired_messenger = pool<messenger>::handle;

    client(boost::asio::io_context& ioc, const std::string& address,
           const std::uint16_t port, const int connections)
        : m_ioc(ioc),
          m_pool(
              m_ioc,
              [&]() {
                  auto endpoint = resolve(address, port).back();
                  return std::make_unique<messenger>(
                      ioc, endpoint.address().to_string(), port);
              },
              connections) {}

    client(client&&) = default;

    coro<acquired_messenger> acquire_messenger() {
        auto ret = co_await m_pool.get();
        co_await ret->ensure_connected();
        co_return std::move(ret);
    }

private:
    boost::asio::io_context& m_ioc;
    pool<messenger> m_pool;
};

} // end namespace uh::cluster
