#pragma once

#include <boost/asio.hpp>

namespace uh::test {

// ---------------------------------------------------------------------

class server {
public:
    server(const std::string& addr, uint16_t port);

    ~server();

private:
    void accept();
    void run();

    boost::asio::io_context m_ioc;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::thread m_thread;
};

// ---------------------------------------------------------------------

} // namespace uh::test
