//
// Created by masi on 8/24/23.
//

#ifndef CORE_MESSENGER_CORE_H
#define CORE_MESSENGER_CORE_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>
#include <list>
#include <common/awaitable_future.h>


namespace uh::cluster {

template <typename T>
using coro =  boost::asio::awaitable <T>;

class messenger_core {
public:

    struct header {
        message_type type;
        uint32_t size;
    };

    messenger_core (const std::shared_ptr <boost::asio::io_context>& ioc, const std::string& address, const int port):
        m_socket (*ioc),
        m_ioc (ioc) {
        boost::asio::ip::tcp::endpoint endpoint (boost::asio::ip::address::from_string (address), port);
        m_socket.connect (endpoint);
    }

    explicit messenger_core (boost::asio::ip::tcp::socket &&socket): m_socket (std::move (socket)) {
    }

    messenger_core (messenger_core&& m) noexcept:
        m_socket(std::move (m.m_socket)),
        m_read_buffers(std::move (m.m_read_buffers)),
        m_write_buffers (std::move (m.m_write_buffers)),
        m_read_size (m.m_read_size),
        m_write_size (m.m_write_size) {
    }



    template <typename T>
    requires (std::is_arithmetic_v <T> or std::is_enum_v <T>)
    inline void register_read_buffer (T& t) {
        m_read_buffers.emplace_back (&t, sizeof (t));
        m_read_size += sizeof (t);
    }

    template <typename T>
    requires (std::is_arithmetic_v <T> or std::is_enum_v <T>)
    inline void register_read_buffer (T* t, std::uint32_t size) {
        m_read_buffers.emplace_back (t, size * sizeof (T));
        m_read_size += size * sizeof (T);
    }

    template <typename T, typename InnerType = std::ranges::range_value_t <T>>
    requires std::ranges::contiguous_range<T>
             and (std::is_arithmetic_v <InnerType>)
    inline void register_read_buffer (T& t) {
        m_read_buffers.emplace_back (std::ranges::data (t), std::ranges::size (t) * sizeof (InnerType));
        m_read_size += std::ranges::size (t) * sizeof(InnerType);
    }

    template<typename T>
    inline void register_read_buffer (const ospan <T>& buf) {
        m_read_buffers.emplace_back (buf.data.get(), buf.size * sizeof(T));
        m_read_size += buf.size * sizeof(T);
    }

    template <typename T>
    requires (std::is_arithmetic_v <T> or std::is_enum_v <T>)
    inline void register_write_buffer (const T& t) {
        m_write_buffers.emplace_back (&t, sizeof (t));
        m_write_size += sizeof (t);
    }

    template <typename T>
    requires (std::is_arithmetic_v <T> or std::is_enum_v <T>)
    inline void register_write_buffer (const T* t, std::uint32_t size) {
        m_write_buffers.emplace_back (t, size * sizeof (T));
        m_write_size += size * sizeof (T);
    }

    template <typename T, typename InnerType = std::ranges::range_value_t <T>>
    requires std::ranges::contiguous_range <T>
             and (std::is_arithmetic_v <InnerType>)
    inline void register_write_buffer (const T& t) {
        m_write_buffers.emplace_back (std::ranges::data (t), std::ranges::size (t) * sizeof (InnerType));
        m_write_size += std::ranges::size (t) * sizeof (InnerType);
    }

    template<typename T>
    inline void register_write_buffer (const ospan <T>& buf) {
        m_write_buffers.emplace_back (buf.data.get(), buf.size * sizeof(T));
        m_write_size += buf.size;
    }

    coro <header> recv_header () {
        header h;
        std::list <boost::asio::mutable_buffer> buffers {
                {&h.type, sizeof h.type},
                {&h.size, sizeof h.size}
        };
        co_await boost::asio::async_read (m_socket, buffers, boost::asio::as_tuple(boost::asio::use_awaitable));

        co_return h;
    }

    coro <void> recv_buffers (const header& h) {
        if (h.size != m_read_size) [[unlikely]] {
            throw std::length_error ("The size of the buffers does not match with the header size!");
        }
        //boost::asio::read (m_socket, m_read_buffers);
        co_await boost::asio::async_read (m_socket, m_read_buffers, boost::asio::as_tuple(boost::asio::use_awaitable));

        m_read_buffers.clear();
        m_read_size = 0;
        co_return;
    }

    coro <void> send_buffers (const message_type type) {
        m_write_buffers.emplace_front(&m_write_size, sizeof m_write_size);
        m_write_buffers.emplace_front(&type, sizeof type);

        //boost::asio::write (m_socket, m_write_buffers);
        co_await boost::asio::async_write (m_socket, m_write_buffers, boost::asio::as_tuple(boost::asio::use_awaitable));

        m_write_buffers.clear();
        m_write_size = 0;
        co_return;

    }

    coro <void> send (const message_type type, std::span <const char> data) {
        const auto size = static_cast <uint32_t> (data.size());

        std::vector <boost::asio::const_buffer> buffers {
                {&type, sizeof (type)},
                {&size, sizeof (size)},
                {data.data(), data.size()}
        };

        co_await boost::asio::async_write (m_socket, buffers, boost::asio::as_tuple(boost::asio::use_awaitable));
        co_return;
    }

    coro <header> recv (std::span <char> buffer) {
        uint32_t size = 0;
        message_type type;
        std::vector <boost::asio::mutable_buffer> buffers {
                {&type, sizeof (type)},
                {&size, sizeof (size)},
                {buffer.data(), buffer.size()}};

        co_await boost::asio::async_read (m_socket, buffers, boost::asio::as_tuple(boost::asio::use_awaitable));

        co_return header {type, size};
    }

    coro <std::pair <header, ospan <char>>> send_recv (message_type type, std::span <const char> data) {
        co_await send (type, data);
        const auto h = co_await recv_header();
        ospan <char> buf (h.size);
        register_read_buffer(buf);
        co_await recv_buffers(h);
        co_return std::move (std::pair {h, std::move (buf)});
    }

    void clear_buffers () {
        m_write_buffers.clear();
        m_read_buffers.clear();
        m_write_size = 0;
        m_read_size = 0;
    }

    ~messenger_core() {
        if(m_socket.is_open()) {
            m_socket.shutdown (boost::asio::ip::tcp::socket::shutdown_both);
            m_socket.close();
        }
    }


private:

    boost::asio::ip::tcp::socket m_socket;

    std::list <boost::asio::mutable_buffer> m_read_buffers;
    std::list <boost::asio::const_buffer> m_write_buffers;
    std::uint32_t m_read_size = 0;
    std::uint32_t m_write_size = 0;

    std::shared_ptr <boost::asio::io_context> m_ioc;

};

} // end namespace uh::cluster


#endif //CORE_MESSENGER_CORE_H
