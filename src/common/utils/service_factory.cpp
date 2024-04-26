#include "service_factory.h"
#include "storage/storage_interface.h"

namespace uh::cluster {

template <>
std::shared_ptr<storage_interface>
service_factory<storage_interface>::make_remote_service(
    const service_endpoint& service) {
    return std::make_shared<remote_storage>(
        coro_client(m_ioc, service.host, service.port, m_connections));
}

} // namespace uh::cluster
