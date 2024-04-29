#include "service_factory.h"
#include "deduplicator/deduplicator_interface.h"
#include "directory/directory_interface.h"
#include "storage/storage_interface.h"

namespace uh::cluster {

template <>
std::shared_ptr<storage_interface>
service_factory<storage_interface>::make_remote_service(
    const service_endpoint& service) {
    return std::make_shared<remote_storage>(
        coro_client(m_ioc, service.host, service.port, m_connections));
}

template <>
std::shared_ptr<deduplicator_interface>
service_factory<deduplicator_interface>::make_remote_service(
    const service_endpoint& service) {
    return std::make_shared<remote_deduplicator>(
        coro_client(m_ioc, service.host, service.port, m_connections));
}

template <>
std::shared_ptr<directory_interface>
service_factory<directory_interface>::make_remote_service(
    const service_endpoint& service) {
    return std::make_shared<remote_directory>(
        coro_client(m_ioc, service.host, service.port, m_connections));
}
} // namespace uh::cluster
