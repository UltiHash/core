#include "service.h"

#include <common/network/tools.h>

#include "handler.h"
#include "request_factory.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/beast/ssl.hpp>

namespace net = boost::asio;
namespace ssl = net::ssl;

namespace uh::cluster::proxy {

using tcp = boost::asio::ip::tcp;

std::unique_ptr<beast::ssl_stream<beast::tcp_stream>>
socket_factory(boost::asio::io_context& ioc, const std::string& server,
               uint16_t port) {

    ssl::context ctx(ssl::context::tls_client);
    ctx.set_default_verify_paths();
    ctx.load_verify_file("/home/sungsik/Projects/core/cert.pem");

    ctx.set_verify_mode(ssl::verify_peer);

    tcp::resolver resolver(ioc);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    auto const results = resolver.resolve(server, std::to_string(port));

    beast::get_lowest_layer(stream).connect(results);

    stream.set_verify_callback(ssl::host_name_verification(server));

    stream.handshake(ssl::stream_base::client);

    return std::make_unique<decltype(stream)>(std::move(stream));
}

service::service(boost::asio::io_context& ioc, const service_config& sc,
                 const config& c)
    : m_ioc(ioc),
      m_etcd(sc.etcd_config),
      m_dv(std::make_unique<storage::global::global_data_view>(ioc, m_etcd,
                                                               c.gdv)),
      m_mgr(cache::disk::manager::create(ioc, *m_dv, 10 * GIBI_BYTE)),
      m_server(c.server,
               std::make_unique<handler<beast::ssl_stream<beast::tcp_stream>>>(
                   std::make_unique<request_factory>(),
                   [this, c] {
                       return socket_factory(m_ioc, c.downstream_host,
                                             c.downstream_port);
                   },
                   *m_dv, m_mgr, c.buffer_size),
               m_ioc) {}

} // namespace uh::cluster::proxy
