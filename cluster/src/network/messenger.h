//
// Created by masi on 8/24/23.
//

#ifndef CORE_MESSENGER_H
#define CORE_MESSENGER_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>


namespace uh::cluster {

class client;

class messenger {
public:
    messenger (boost::asio::io_context& ioc, const std::string& address, const int port):
        m_socket (ioc),
        m_strand (ioc) {
        boost::asio::ip::tcp::endpoint endpoint (boost::asio::ip::address::from_string (address), port);
        m_socket.connect (endpoint);
    }

    explicit messenger (boost::asio::io_context& ioc, boost::asio::ip::tcp::socket &&socket): m_socket (std::move (socket)), m_strand (ioc) {
    }

    messenger (messenger&& m) noexcept: m_socket(std::move (m.m_socket)), m_strand (m.m_strand) {
    }

    std::pair <message_types, ospan <char>> recv () {
        uint32_t size = 0;
        message_types type;
        std::vector <boost::asio::mutable_buffer> header {
                {&size, sizeof (size)},
                {&type, sizeof (type)},
        };

        ospan <char> buf;
        boost::asio::co_spawn (m_strand.context(), [&] () -> boost::asio::awaitable <void> {
            co_await boost::asio::async_read (m_socket, header, boost::asio::as_tuple(boost::asio::use_awaitable));
            buf.resize(size);
            co_await boost::asio::async_read (m_socket, boost::asio::mutable_buffer (buf.data.get(), size), boost::asio::as_tuple(boost::asio::use_awaitable));
        }, [] (const std::exception_ptr& e) {if (e) std::rethrow_exception(e);});

        return {type, std::move (buf)};
    }

    std::pair <message_types, size_t> recv (std::span <char> buffer) {
        uint32_t size = 0;
        message_types type;
        std::vector <boost::asio::mutable_buffer> buffers {
                {&size, sizeof (size)},
                {&type, sizeof (type)},
                {buffer.data(), buffer.size()}};

        boost::asio::co_spawn (m_strand.context(), [&] () -> boost::asio::awaitable <void> {
            co_await boost::asio::async_read (m_socket, buffers, boost::asio::as_tuple(boost::asio::use_awaitable));
        }, [] (const std::exception_ptr& e) {if (e) std::rethrow_exception(e);});
        return {type, size};
    }

    void send (const message_types type, std::span <const char> data) {
        const auto size = static_cast <uint32_t> (data.size());

        std::vector <boost::asio::const_buffer> send_data {
                {&type, sizeof (type)},
                {&size, sizeof (size)},
                {data.data(), data.size()},
        };

        boost::asio::co_spawn (m_strand.context(), [&] () -> boost::asio::awaitable <void> {
            co_await boost::asio::async_write (m_socket, send_data, boost::asio::as_tuple(boost::asio::use_awaitable));
        }, [] (const std::exception_ptr& e) {if (e) std::rethrow_exception(e);});
    }

    ~messenger() {
        m_socket.shutdown (boost::asio::ip::tcp::socket::shutdown_send);
    }

private:


    friend client;
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::io_context::strand m_strand;
};

} // end namespace uh::cluster


#endif //CORE_MESSENGER_H
