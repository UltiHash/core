//
// Created by masi on 8/24/23.
//

#ifndef CORE_MESSENGER_CORE_H
#define CORE_MESSENGER_CORE_H

#include "common/utils/error.h"
#include "common/utils/common.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio.hpp>
#include <list>


namespace uh::cluster {

    template <typename T>
    using coro =  boost::asio::awaitable <T>;

    using size_type = size_t;

    class messenger_core {


    public:
        struct header {
            message_type type;
            size_type size;
        };

        messenger_core(const std::shared_ptr <boost::asio::io_context>& ioc, const std::string& ip_addr,
                       const std::uint16_t port) :
                m_socket (*ioc),
                m_ioc (ioc)
        {
            boost::asio::ip::tcp::endpoint endpoint (boost::asio::ip::address::from_string (ip_addr), port);
            m_socket.connect (endpoint);
            clear_buffers();
        }

        explicit messenger_core (boost::asio::ip::tcp::socket &&socket): m_socket (std::move (socket)) {
            clear_buffers();
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
            m_read_buffers.emplace_back (&t, sizeof (T));
            m_read_size += sizeof (T);
        }

        template <typename T>
        requires (std::is_arithmetic_v <T> or std::is_enum_v <T>)
        inline void register_read_buffer (T* t, size_type size) {
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
        inline void register_read_buffer (const unique_buffer <T>& buf) {
            m_read_buffers.emplace_back (buf.data(), buf.size() * sizeof(T));
            m_read_size += buf.size() * sizeof(T);
        }

        template <typename T>
        requires (std::is_arithmetic_v <T> or std::is_enum_v <T>)
        inline void register_write_buffer (const T& t) {
            m_write_buffers.emplace_back (&t, sizeof (T));
            m_write_size += sizeof (T);
        }

        template <typename T>
        requires (std::is_arithmetic_v <T> or std::is_enum_v <T>)
        inline void register_write_buffer (const T* t, size_type size) {
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
        inline void register_write_buffer (const unique_buffer <T>& buf) {
            m_write_buffers.emplace_back (buf.data(), buf.size() * sizeof(T));
            m_write_size += buf.size();
        }

        coro <header> recv_header () {
            header h;
            std::list <boost::asio::mutable_buffer> buffers {
                    {&h.type, sizeof h.type},
                    {&h.size, sizeof h.size}
            };

            co_await boost::asio::async_read (m_socket, buffers, boost::asio::as_tuple(boost::asio::use_awaitable));

            if (h.type == FAILURE) [[unlikely]] {
                const auto e = co_await recv_error(h);
                throw error_exception(e);
            }

            co_return h;
        }

        coro <void> recv_buffers (const header& h) {
            if (h.size != m_read_size) [[unlikely]] {
                throw std::length_error ("The size of the buffers does not match with the header size!");
            }

            co_await boost::asio::async_read (m_socket, m_read_buffers, boost::asio::as_tuple(boost::asio::use_awaitable));

            m_read_buffers.clear();
            m_read_size = 0;
        }

        void reserve_write_buffers (size_t capacity) {
            m_write_buffers.reserve(capacity + 2);
        }

        void reserve_read_buffers (size_t capacity) {
            m_read_buffers.reserve(capacity);
        }

        void reset_write_buffers () {
            m_write_buffers.clear();
            m_write_size = 0;
            m_write_buffers.emplace_back();
            m_write_buffers.emplace_back();
        }

        void reset_read_buffers () {
            m_read_buffers.clear();
            m_read_size = 0;
        }

        coro <void> send_buffers (const message_type type) {
            m_write_buffers[0] = {&type, sizeof type};
            m_write_buffers[1] = {&m_write_size, sizeof m_write_size};

            co_await boost::asio::async_write (m_socket, m_write_buffers, boost::asio::as_tuple(boost::asio::use_awaitable));

            reset_write_buffers();
        }

        coro <void> send_error (const error& e) {
            const auto ec = e.code();
            register_write_buffer(ec);
            register_write_buffer(e.message());
            co_await send_buffers(FAILURE);
        }

        coro <error> recv_error (const header& h) {
            uint32_t ec;
            std::string msg (h.size - sizeof(ec), 0);
            register_read_buffer (ec);
            register_read_buffer (msg);
            co_await recv_buffers(h);
            co_return error (ec, msg);
        }

        coro <void> send (const message_type type, std::span <const char> data) {
            const auto size = static_cast <size_type> (data.size());

            std::vector <boost::asio::const_buffer> buffers {
                    {&type, sizeof (type)},
                    {&size, sizeof (size)},
                    {data.data(), data.size()}
            };

            co_await boost::asio::async_write (m_socket, buffers, boost::asio::as_tuple(boost::asio::use_awaitable));
            co_return;
        }

        void clear_buffers () {
            reset_write_buffers();
            reset_read_buffers();
        }

        ~messenger_core() {
            if(m_socket.is_open()) {
                m_socket.shutdown (boost::asio::ip::tcp::socket::shutdown_both);
                m_socket.close();
            }
        }


    private:

        boost::asio::ip::tcp::socket m_socket;

        std::vector <boost::asio::mutable_buffer> m_read_buffers;
        std::vector <boost::asio::const_buffer> m_write_buffers;
        size_type m_read_size = 0;
        size_type m_write_size = 0;

        std::shared_ptr <boost::asio::io_context> m_ioc;

    };

} // end namespace uh::cluster


#endif //CORE_MESSENGER_CORE_H
