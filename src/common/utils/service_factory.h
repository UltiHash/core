
#ifndef UH_CLUSTER_SERVICE_FACTORY_H
#define UH_CLUSTER_SERVICE_FACTORY_H

#include "common/registry/attached_service.h"
#include "common/utils/host_utils.h"
#include <boost/asio/io_context.hpp>
#include <memory>

namespace uh::cluster {

struct service_endpoint {
    std::size_t id{};
    std::string host{};
    std::uint16_t port{};
    int pid{};
};

template <typename service_interface> struct service_factory {

public:
    service_factory(boost::asio::io_context& ioc, int connections,
                    std::shared_ptr<service_interface> local_service)
        : m_ioc(ioc),
          m_connections(connections),
          m_local_service(std::move(local_service)) {}

    std::shared_ptr<service_interface>
    make_service(const service_endpoint& service) {
        if (!m_local_service or service.host != get_host() or
            service.pid != getpid()) {
            return make_remote_service(service);
        }
        return m_local_service;
    }

    std::shared_ptr<service_interface> get_local_service() const {
        return m_local_service;
    }

    virtual ~service_factory() = default;

private:
    std::shared_ptr<service_interface>
    make_remote_service(const service_endpoint& service);

    boost::asio::io_context& m_ioc;
    const int m_connections;
    std::shared_ptr<service_interface> m_local_service;
};

} // namespace uh::cluster
#endif // UH_CLUSTER_SERVICE_FACTORY_H
