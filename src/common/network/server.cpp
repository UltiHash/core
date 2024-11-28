#include "common/network/server.h"

#include "common/telemetry/metrics.h"

namespace uh::cluster {

boost::asio::ip::tcp::acceptor
server::do_listen(const boost::asio::ip::tcp::endpoint& endpoint) {
    auto acceptor = boost::asio::use_awaitable_t<boost::asio::any_io_executor>::
        as_default_on(boost::asio::ip::tcp::acceptor(m_ioc));

    acceptor.open(endpoint.protocol());
    acceptor.set_option(boost::asio::socket_base::reuse_address(true));

    acceptor.bind(endpoint);
    acceptor.listen(boost::asio::socket_base::max_listen_connections);
    return acceptor;
}

coro<void> server::do_accept(auto acceptor) {
    while (m_is_running) {
        boost::asio::ip::tcp::socket stream =
            co_await acceptor.async_accept(boost::asio::use_awaitable);
        const auto conn_address =
            stream.remote_endpoint().address().to_string();
        const auto conn_port = stream.remote_endpoint().port();

        boost::asio::co_spawn(
            m_ioc, do_session(std::move(stream)),
            [conn_address, conn_port](const std::exception_ptr& e) {
                if (e)
                    try {
                        std::rethrow_exception(e);
                    } catch (const std::exception& e) {
                        LOG_ERROR() << "in session: [" << conn_address << ":"
                                    << conn_port << "] " << e.what();
                    }
            });
    }
}

coro<void> server::do_session(boost::asio::ip::tcp::socket stream) {
    LOG_INFO() << "connection from: " << stream.remote_endpoint();
    counter_guard<active_connections> guard;
    co_await m_handler->handle(std::move(stream));
    co_return;
}

} // namespace uh::cluster
