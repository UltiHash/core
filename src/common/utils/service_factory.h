
#ifndef UH_CLUSTER_SERVICE_FACTORY_H
#define UH_CLUSTER_SERVICE_FACTORY_H

#include "common/registry/attached_service.h"
#include "common/utils/host_utils.h"
#include <boost/asio/io_context.hpp>
#include <memory>

namespace uh::cluster {


template <typename service_interface> struct service_factory {

public:
    service_factory(boost::asio::io_context& ioc, int connections,
                    std::shared_ptr<service_interface> local_service)
        : m_ioc(ioc),
          m_connections(connections),
          m_local_service(std::move(local_service)) {}

    std::shared_ptr<service_interface>
    make_service(const std::string& hostname, uint16_t port, int pid) {
        if (!m_local_service or hostname != get_host() or
            pid != getpid()) {
            return make_remote_service(hostname, port);
        }
        return m_local_service;
    }

    std::shared_ptr<service_interface> get_local_service() const {
        return m_local_service;
    }

    virtual ~service_factory() = default;

private:
    std::shared_ptr<service_interface>
    make_remote_service(const std::string& hostname, uint16_t port);

    boost::asio::io_context& m_ioc;
    const int m_connections;
    std::shared_ptr<service_interface> m_local_service;
};

} // namespace uh::cluster
#endif // UH_CLUSTER_SERVICE_FACTORY_H
