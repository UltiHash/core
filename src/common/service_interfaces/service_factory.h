#pragma once

#include <boost/asio.hpp>
#include <memory>

namespace uh::cluster {

template <typename service_interface> struct service_factory {
public:
    service_factory(boost::asio::io_context& ioc, int connections)
        : m_ioc(ioc),
          m_connections(connections) {}

    std::shared_ptr<service_interface> make_service(const std::string& hostname,
                                                    uint16_t port) {
        return make_remote_service(hostname, port);
    }

private:
    std::shared_ptr<service_interface>
    make_remote_service(const std::string& hostname, uint16_t port);

    boost::asio::io_context& m_ioc;
    int m_connections;
};

} // namespace uh::cluster
