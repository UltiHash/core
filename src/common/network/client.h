#ifndef CORE_CLIENT_H
#define CORE_CLIENT_H

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

    coro<acquired_messenger> acquire_messenger() { return m_pool.get(); }

private:
    boost::asio::io_context& m_ioc;
    pool<messenger> m_pool;
};

} // end namespace uh::cluster

#endif // CORE_CLIENT_H
