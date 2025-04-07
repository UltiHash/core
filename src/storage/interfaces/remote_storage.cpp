#include "remote_storage.h"

namespace uh::cluster {

template <>
std::shared_ptr<storage_interface>
service_factory<storage_interface>::make_remote_service(
    const std::string& hostname, uint16_t port) {
    return std::make_shared<remote_storage>(
        client(m_ioc, hostname, port, m_connections));
}

} // namespace uh::cluster
