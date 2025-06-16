#include "messenger_core.h"
#include <common/telemetry/trace/trace.h>

namespace uh::cluster {

messenger_core::messenger_core(boost::asio::io_context& ioc,
                               const std::string& ip_addr,
                               const std::uint16_t port)
    : m_tcp_stream(ioc) {
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::make_address(ip_addr), port);
    try {
        m_tcp_stream.connect(endpoint);
    } catch (const std::exception& e) {
        throw create_internal_network_error("socket connection failed", e);
    }
    clear_buffers();
}

messenger_core::messenger_core(boost::asio::ip::tcp::socket&& socket)
    : m_tcp_stream(std::move(socket)) {
    clear_buffers();
}

messenger_core::messenger_core(messenger_core&& m) noexcept
    : m_tcp_stream(std::move(m.m_tcp_stream)),
      m_read_buffers(std::move(m.m_read_buffers)),
      m_write_buffers(std::move(m.m_write_buffers)),
      m_read_size(m.m_read_size),
      m_write_size(m.m_write_size) {}

coro<messenger_core::header> messenger_core::recv_header() {
    header h;
    std::string ctx_buffer;
    ctx_buffer.resize(get_encoded_context_len());

    try {
        std::vector<boost::asio::mutable_buffer> buffers{
            {&h.type, sizeof h.type},
            {&h.size, sizeof h.size},
            boost::asio::buffer(ctx_buffer)};

        m_tcp_stream.expires_after(
            time_settings::instance().get_async_io_timeout());
        co_await boost::asio::async_read(m_tcp_stream, buffers,
                                         boost::asio::use_awaitable);
    } catch (const std::exception& e) {
        throw create_internal_network_error("recv_header failed", e);
    }

    h.peer = peer();

    if (h.type == FAILURE) {
        const auto e = co_await recv_error(h);
        throw error_exception(e);
    }

    if (h.type != SUCCESS) {
        measure_message_type(h.type);
    }

    co_return h;
}

coro<std::tuple<messenger_core::header, opentelemetry::context::Context>>
messenger_core::recv_header_with_context() {
    header h;
    std::string ctx_buffer;
    ctx_buffer.resize(get_encoded_context_len());

    try {
        std::vector<boost::asio::mutable_buffer> buffers{
            {&h.type, sizeof h.type},
            {&h.size, sizeof h.size},
            boost::asio::buffer(ctx_buffer)};

        // NOTE: didn't put expires_after here, since the async_read below will
        // wait upstream's write
        co_await boost::asio::async_read(m_tcp_stream, buffers,
                                         boost::asio::use_awaitable);
    } catch (const std::exception& e) {
        throw create_internal_network_error("recv_header failed", e);
    }

    h.peer = peer();

    if (h.type == FAILURE) {
        const auto e = co_await recv_error(h);
        throw error_exception(e);
    }

    if (h.type != SUCCESS) {
        measure_message_type(h.type);
    }
    auto context = decode_context(ctx_buffer);

    if (boost::asio::trace_span::enable &&
        !boost::asio::trace_span::check_context(context)) {
        LOG_ERROR() << "[messenger_core::send] The decoded context is invalid: "
                       "see following stack trace";
        auto span = co_await boost::asio::this_coro::span;
        span->iterate_call_stack(
            [](boost::source_location loc) { LOG_INFO() << loc; });
        LOG_ERROR() << "End of stack trace";
    }

    co_return std::make_tuple(h, context);
}

coro<void> messenger_core::recv_buffers(const messenger_core::header& h) {
    if (h.size != m_read_size) {
        throw std::length_error(
            "The size of the buffers does not match with the header size: " +
            std::to_string(h.size) + " != " + std::to_string(m_read_size));
    }

    try {
        m_tcp_stream.expires_after(
            time_settings::instance().get_async_io_timeout());
        co_await boost::asio::async_read(m_tcp_stream, m_read_buffers,
                                         boost::asio::use_awaitable);
        m_read_buffers.clear();
        m_read_size = 0;

    } catch (const std::exception& e) {
        throw create_internal_network_error("recv_buffers failed", e);
    }
}

void messenger_core::reserve_write_buffers(size_t capacity) {
    m_write_buffers.reserve(capacity + 3);
}

void messenger_core::reserve_read_buffers(size_t capacity) {
    m_read_buffers.reserve(capacity);
}

void messenger_core::reset_write_buffers() {
    m_write_buffers.clear();
    m_write_size = 0;
    m_write_buffers.emplace_back();
    m_write_buffers.emplace_back();
    m_write_buffers.emplace_back();
}

void messenger_core::reset_read_buffers() {
    m_read_buffers.clear();
    m_read_size = 0;
}

coro<void> messenger_core::send_buffers(const message_type type) {
    try {
        if (type == SUCCESS) {
            metric<success>::increase(1);
        }

        auto context = co_await boost::asio::this_coro::context;

        if (boost::asio::trace_span::enable &&
            !boost::asio::trace_span::check_context(context)) {
            LOG_ERROR() << "[messenger_core::send_buffers] The context to be "
                           "encoded is invalid: see following stack trace";
            auto span = co_await boost::asio::this_coro::span;
            span->iterate_call_stack(
                [](boost::source_location loc) { LOG_INFO() << loc; });
            LOG_ERROR() << "End of stack trace";
        }

        auto ctx_buf = encode_context(context);

        m_write_buffers[0] = {&type, sizeof type};
        m_write_buffers[1] = {&m_write_size, sizeof m_write_size};
        m_write_buffers[2] = boost::asio::buffer(ctx_buf);

        m_tcp_stream.expires_after(
            time_settings::instance().get_async_io_timeout());
        co_await boost::asio::async_write(m_tcp_stream, m_write_buffers,
                                          boost::asio::use_awaitable);

    } catch (const std::exception& e) {
        throw create_internal_network_error("send_buffers failed", e);
    }

    reset_write_buffers();
}

coro<void> messenger_core::send_error(const error& e) {
    const auto ec = e.code();
    register_write_buffer(ec);
    register_write_buffer(e.message());
    metric<failure>::increase(1);

    co_await send_buffers(FAILURE);
}

coro<error> messenger_core::recv_error(const messenger_core::header& h) {
    uint32_t ec;
    std::string msg(h.size - sizeof(ec), 0);
    register_read_buffer(ec);
    register_read_buffer(msg);
    co_await recv_buffers(h);
    co_return error(ec, msg);
}

coro<void> messenger_core::send(const message_type type,
                                std::span<const char> data) {
    try {
        if (type == SUCCESS) {
            metric<success>::increase(1);
        }

        auto size = static_cast<size_type>(data.size());

        auto context = co_await boost::asio::this_coro::context;

        if (boost::asio::trace_span::enable &&
            !boost::asio::trace_span::check_context(context)) {
            LOG_ERROR() << "[messenger_core::recv_header] The context to be "
                           "encoded is invalid: see following stack trace";
            auto span = co_await boost::asio::this_coro::span;
            span->iterate_call_stack(
                [](boost::source_location loc) { LOG_INFO() << loc; });
            LOG_ERROR() << "End of stack trace";
        }

        auto ctx_buf = encode_context(context);

        std::vector<boost::asio::const_buffer> buffers{
            {&type, sizeof(type)},
            {&size, sizeof(size)},
            boost::asio::buffer(ctx_buf),
            {data.data(), data.size()}};

        m_tcp_stream.expires_after(
            time_settings::instance().get_async_io_timeout());
        co_await boost::asio::async_write(m_tcp_stream, buffers,
                                          boost::asio::use_awaitable);
    } catch (const std::exception& e) {
        throw create_internal_network_error("send failed", e);
    }
}

void messenger_core::clear_buffers() {
    reset_write_buffers();
    reset_read_buffers();
}

boost::asio::ip::tcp::endpoint messenger_core::local() const {
    return m_tcp_stream.socket().local_endpoint();
}

boost::asio::ip::tcp::endpoint messenger_core::peer() const {
    return m_tcp_stream.socket().remote_endpoint();
}

boost::asio::ip::tcp::socket& messenger_core::get_socket() noexcept {
    return m_tcp_stream.socket();
}

error_exception
messenger_core::create_internal_network_error(const std::string& message,
                                              const std::exception& e) {
    return error_exception(
        error(error::internal_network_error, message + " | " + e.what()));
}

} // namespace uh::cluster
