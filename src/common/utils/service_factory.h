
#ifndef UH_CLUSTER_SERVICE_FACTORY_H
#define UH_CLUSTER_SERVICE_FACTORY_H

#include "storage/storage_interface.h"
#include <boost/asio/io_context.hpp>
#include <memory>

namespace uh::cluster {

template <typename service_interface>
struct service_factory {

public:
    service_factory(boost::asio::io_context& ioc, int connections, std::shared_ptr <service_interface> local_service = nullptr):
          m_ioc (ioc),
          m_connections (connections),
          m_local_service (std::move (local_service))
    {
    }
    virtual std::shared_ptr<service_interface> make_service(const std::string& address, std::uint16_t port) = 0;

    virtual std::shared_ptr<service_interface> make_service() {
        if (!m_local_service) {
            throw std::logic_error("No local services available");
        }
        return m_local_service;
    }

    virtual ~service_factory() = default;

protected:

    boost::asio::io_context& m_ioc;
    const int m_connections;
    std::shared_ptr <service_interface> m_local_service;
};


struct storage_factory: public service_factory <storage_interface> {

    using service_factory <storage_interface>::service_factory;
    std::shared_ptr<storage_interface> make_service(const std::string& address, std::uint16_t port) override {
        return std::make_shared<remote_storage>(coro_client (m_ioc, address, port, m_connections));
    }

};
}
#endif // UH_CLUSTER_SERVICE_FACTORY_H
