//
// Created by masi on 8/24/23.
//

#ifndef CORE_MESSENGER_H
#define CORE_MESSENGER_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>


namespace uh::cluster {

class messenger {
public:
    explicit messenger (boost::asio::io_service& io_service, const std::string& address, const int port): m_socket (io_service) {
        boost::asio::ip::tcp::endpoint endpoint (boost::asio::ip::address::from_string (address), port);
        m_socket.connect (endpoint);
    }

    explicit messenger (boost::asio::ip::tcp::socket &&socket): m_socket (std::move (socket)) {

    }

    messenger (messenger&& m) noexcept: m_socket(std::move (m.m_socket)) {

    }

    void send (std::span <char> data) {
        const auto size = static_cast <uint32_t> (data.size());
        m_socket.send(boost::asio::const_buffer (&size, sizeof (size)));
        m_socket.send(boost::asio::const_buffer (data.data(), data.size()));
    }

    boost::asio::awaitable <ospan <char>> recv () {
        uint32_t size = 0;
        co_await boost::asio::async_read (m_socket, boost::asio::mutable_buffer (&size, sizeof (size)), boost::asio::as_tuple(boost::asio::use_awaitable));
        ospan <char> buf (size);
        co_await boost::asio::async_read (m_socket, boost::asio::mutable_buffer (buf.data.get(), size), boost::asio::as_tuple(boost::asio::use_awaitable));
        co_return buf;
    }

    boost::asio::awaitable <size_t> recv (std::span <char> buffer) {
        uint32_t size = 0;
        co_await boost::asio::async_read (m_socket, boost::asio::mutable_buffer (&size, sizeof (size)), boost::asio::as_tuple(boost::asio::use_awaitable));
        if (size > buffer.size()) [[unlikely]] {
            throw std::overflow_error ("Buffer overflow when receiving the data!");
        }
        co_await boost::asio::async_read (m_socket, boost::asio::mutable_buffer (buffer.data(), size), boost::asio::as_tuple(boost::asio::use_awaitable));
        co_return size;
    }

private:

    boost::asio::ip::tcp::socket m_socket;
};

} // end namespace uh::cluster


#endif //CORE_MESSENGER_H
