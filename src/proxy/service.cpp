#include "service.h"

#include <common/network/tools.h>

#include "request_factory.h"
#include "handler.h"

#include <boost/asio.hpp>


namespace uh::cluster::proxy {

using tcp = boost::asio::ip::tcp;

std::unique_ptr<tcp::socket> socket_factory(
    boost::asio::io_context& ioc,
    const std::string& server,
    uint16_t port) {

    auto addr = uh::cluster::resolve(server, port);
    if (addr.empty()) {
        throw std::runtime_error("lookup failed");
    }

    tcp::socket s(ioc);
    boost::asio::connect(s, addr);

    return std::make_unique<tcp::socket>(std::move(s));
}

service::service(boost::asio::io_context& ioc, const service_config& sc, const config& c)
    : m_ioc(ioc),
      m_etcd(sc.etcd_config),
      m_dv(std::make_unique<storage::global::global_data_view>(ioc, m_etcd, c.gdv)),
      m_mgr(cache::disk::manager::create(ioc, *m_dv, 10 * GIBI_BYTE)),
      m_server(c.server, std::make_unique<handler>(
        std::make_unique<request_factory>(),
        [this, c]{ return socket_factory(m_ioc, c.downstream_host, c.downstream_port); },
        *m_dv, m_mgr,
        c.buffer_size),
              m_ioc) {
}

} // namespace uh::cluster::proxy
