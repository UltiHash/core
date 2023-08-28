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
    messenger (boost::asio::io_service& io_service, const std::string& address, const int port):
        m_socket (io_service) {
        boost::asio::ip::tcp::endpoint endpoint (boost::asio::ip::address::from_string (address), port);
        m_socket.connect (endpoint);
    }

    explicit messenger (boost::asio::ip::tcp::socket &&socket): m_socket (std::move (socket)){
    }

    messenger (messenger&& m) noexcept: m_socket(std::move (m.m_socket)) {
    }

    boost::asio::awaitable <std::pair <message_types, ospan <char>>> recv () {
        uint32_t size = 0;
        message_types type;
        std::vector <boost::asio::mutable_buffer> header {
                {&size, sizeof (size)},
                {&type, sizeof (type)},
        };
        co_await boost::asio::async_read (m_socket, header, boost::asio::as_tuple(boost::asio::use_awaitable));
        ospan <char> buf (size);
        co_await boost::asio::async_read (m_socket, boost::asio::mutable_buffer (buf.data.get(), size), boost::asio::as_tuple(boost::asio::use_awaitable));
        co_return std::pair <message_types, ospan<char>> (type, std::move (buf));
    }

    boost::asio::awaitable <std::pair <message_types, size_t>> recv (std::span <char> buffer) {
        uint32_t size = 0;
        message_types type;
        std::vector <boost::asio::mutable_buffer> buffers {
                {&size, sizeof (size)},
                {&type, sizeof (type)},
                {buffer.data(), buffer.size()}};
        co_await boost::asio::async_read (m_socket, buffers, boost::asio::as_tuple(boost::asio::use_awaitable));
        co_return std::pair <message_types, std::size_t> (type, size);
    }

private:

    boost::asio::awaitable<void> send (const message_types type, std::span <const char> data) {
        const auto size = static_cast <uint32_t> (data.size());

        std::vector <boost::asio::const_buffer> send_data {
                {&type, sizeof (type)},
                {&size, sizeof (size)},
                {data.data(), data.size()},
        };

        co_await boost::asio::async_write(m_socket, send_data, boost::asio::as_tuple(boost::asio::use_awaitable));
    }

    friend client;
    boost::asio::ip::tcp::socket m_socket;
};

} // end namespace uh::cluster


#endif //CORE_MESSENGER_H
