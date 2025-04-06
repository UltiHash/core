#include "server.h"

using namespace boost::asio;

namespace uh::test {

namespace {

ip::tcp::acceptor acceptor(io_context& ioc, const std::string& addr,
                           uint16_t port) {
    ip::tcp::acceptor acceptor(ioc);

    acceptor.open(ip::tcp::v4());
    acceptor.set_option(socket_base::reuse_address(true));
    acceptor.bind(ip::tcp::endpoint{ip::make_address(addr), port});
    acceptor.listen(16);

    return acceptor;
}

} // namespace

server::server(const std::string& addr, uint16_t port)
    : m_ioc(),
      m_acceptor(acceptor(m_ioc, addr, port)),
      m_thread([this]() { run(); }) {}

server::~server() {
    m_ioc.stop();
    m_thread.join();
}

void server::accept() {
    auto sock = std::make_shared<ip::tcp::socket>(m_ioc);
    m_acceptor.async_accept(*sock, [this, sock](auto err) { accept(); });
}

void server::run() {
    accept();
    m_ioc.run();
}

} // namespace uh::test
