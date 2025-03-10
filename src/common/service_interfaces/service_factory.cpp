#include "service_factory.h"
#include "deduplicator/interfaces/remote_deduplicator.h"
#include "storage/interfaces/remote_storage.h"

namespace uh::cluster {

template <>
std::shared_ptr<sn::interface>
service_factory<sn::interface>::make_remote_service(const std::string& hostname,
                                                    uint16_t port) {
    return std::make_shared<remote_storage>(
        client(m_ioc, hostname, port, m_connections));
}

template <>
std::shared_ptr<deduplicator_interface>
service_factory<deduplicator_interface>::make_remote_service(
    const std::string& hostname, uint16_t port) {
    return std::make_shared<remote_deduplicator>(
        client(m_ioc, hostname, port, m_connections));
}

} // namespace uh::cluster
