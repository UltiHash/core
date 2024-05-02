#include "service_factory.h"
#include "deduplicator/interfaces/remote_deduplicator.h"
#include "directory/interfaces/remote_directory.h"
#include "storage/interfaces/remote_storage.h"

namespace uh::cluster {

template <>
std::shared_ptr<storage_interface>
service_factory<storage_interface>::make_remote_service(
    const service_endpoint& service) {
    return std::make_shared<remote_storage>(
        client(m_ioc, service.host, service.port, m_connections));
}

template <>
std::shared_ptr<deduplicator_interface>
service_factory<deduplicator_interface>::make_remote_service(
    const service_endpoint& service) {
    return std::make_shared<remote_deduplicator>(
        client(m_ioc, service.host, service.port, m_connections));
}

template <>
std::shared_ptr<directory_interface>
service_factory<directory_interface>::make_remote_service(
    const service_endpoint& service) {
    return std::make_shared<remote_directory>(
        client(m_ioc, service.host, service.port, m_connections));
}
} // namespace uh::cluster
