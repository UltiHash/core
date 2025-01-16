#pragma once

#include "common/telemetry/context.h"
#include "common/telemetry/log.h"
#include "common/telemetry/metrics.h"
#include "common/types/common_types.h"
#include "common/types/scoped_buffer.h"
#include "common/utils/common.h"
#include "common/utils/error.h"

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace uh::cluster {

using size_type = size_t;

class messenger_core {

public:
    struct header {
        message_type type;
        size_type size;

        context ctx;
    };

    messenger_core(boost::asio::io_context& ioc, const std::string& ip_addr,
                   const std::uint16_t port);

    explicit messenger_core(boost::asio::ip::tcp::socket&& socket);

    messenger_core(messenger_core&& m) noexcept;

    template <typename T>
    requires(std::is_arithmetic_v<T> or std::is_enum_v<T>)
    inline void register_read_buffer(T& t) {
        m_read_buffers.emplace_back(&t, sizeof(T));
        m_read_size += sizeof(T);
    }

    template <typename T>
    requires(std::is_arithmetic_v<T> or std::is_enum_v<T>)
    inline void register_read_buffer(T* t, size_type size) {
        m_read_buffers.emplace_back(t, size * sizeof(T));
        m_read_size += size * sizeof(T);
    }

    template <typename T, typename InnerType = std::ranges::range_value_t<T>>
    requires std::ranges::contiguous_range<T> and
             (std::is_arithmetic_v<InnerType>)
    inline void register_read_buffer(T& t) {
        m_read_buffers.emplace_back(std::ranges::data(t),
                                    std::ranges::size(t) * sizeof(InnerType));
        m_read_size += std::ranges::size(t) * sizeof(InnerType);
    }

    template <typename T>
    inline void register_read_buffer(const unique_buffer<T>& buf) {
        m_read_buffers.emplace_back(buf.data(), buf.size() * sizeof(T));
        m_read_size += buf.size() * sizeof(T);
    }

    template <typename T>
    requires(std::is_arithmetic_v<T> or std::is_enum_v<T>)
    inline void register_write_buffer(const T& t) {
        m_write_buffers.emplace_back(&t, sizeof(T));
        m_write_size += sizeof(T);
    }

    template <typename T>
    requires(std::is_arithmetic_v<T> or std::is_enum_v<T>)
    inline void register_write_buffer(const T* t, size_type size) {
        m_write_buffers.emplace_back(t, size * sizeof(T));
        m_write_size += size * sizeof(T);
    }

    template <typename T, typename InnerType = std::ranges::range_value_t<T>>
    requires std::ranges::contiguous_range<T> and
             (std::is_arithmetic_v<InnerType>)
    inline void register_write_buffer(const T& t) {
        m_write_buffers.emplace_back(std::ranges::data(t),
                                     std::ranges::size(t) * sizeof(InnerType));
        m_write_size += std::ranges::size(t) * sizeof(InnerType);
    }

    template <typename T>
    inline void register_write_buffer(const unique_buffer<T>& buf) {
        m_write_buffers.emplace_back(buf.data(), buf.size() * sizeof(T));
        m_write_size += buf.size();
    }

    coro<header> recv_header();

    coro<void> recv_buffers(const header& h);

    void reserve_write_buffers(size_t capacity);

    void reserve_read_buffers(size_t capacity);

    void reset_write_buffers();

    void reset_read_buffers();

    coro<void> send_buffers(context& ctx, const message_type type);

    coro<void> send_error(context& ctx, const error& e);

    coro<error> recv_error(const header& h);

    coro<void> send(context& ctx, const message_type type,
                    std::span<const char> data);

    void clear_buffers();

    boost::asio::ip::tcp::endpoint local() const;
    boost::asio::ip::tcp::endpoint peer() const;
    boost::asio::ip::tcp::socket& get_socket() noexcept;

    ~messenger_core();

private:
    boost::asio::ip::tcp::socket m_socket;

    std::vector<boost::asio::mutable_buffer> m_read_buffers;
    std::vector<boost::asio::const_buffer> m_write_buffers;
    size_type m_read_size = 0;
    size_type m_write_size = 0;

    error_exception create_internal_network_error(const std::string& message,
                                                  const std::exception& e);
};

} // end namespace uh::cluster
