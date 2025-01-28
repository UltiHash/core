#include "remote_storage.h"

namespace uh::cluster {

remote_factory::remote_factory(boost::asio::io_context& ioc,
                               std::size_t connections)
    : m_ioc(ioc),
      m_connections(connections) {}

std::shared_ptr<remote_storage>
remote_factory::make_service(const std::string& hostname, uint16_t port, int) {
    return std::make_shared<remote_storage>(
        client(m_ioc, hostname, port, m_connections));
}

} // namespace uh::cluster
