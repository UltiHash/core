#include "forward_command.h"

#include <entrypoint/http/raw_body.h>

namespace uh::cluster::proxy {

forward_command::forward_command(boost::beast::tcp_stream& to,
                                 std::size_t buffer_size)
    : m_to(to),
      m_buffer_size(buffer_size) {
}

coro<void> forward_command::validate(const ep::http::request& r) {

    beast::http::request_serializer<beast::http::empty_body> ser(r.base());

    LOG_INFO() << r.peer() << ": writing headers";
    co_await beast::http::async_write_header(m_to, ser);

    if (auto expect = r.header("expect"); expect && *expect == "100-continue") {

        m_to.expires_after(m_expected_timeout);

        beast::http::response_parser<beast::http::empty_body> pr;
        pr.body_limit((std::numeric_limits<std::uint64_t>::max)());

        co_await beast::http::async_read_header(m_to, m_downstream_buffer, pr);
        m_to.expires_never();

        if (pr.get().result() != boost::beast::http::status::continue_) {
            throw command_exception(pr.get().result(), pr.get().reason(), pr.get().reason());
        }
    }

    co_return;
}

coro<ep::http::response> forward_command::handle(ep::http::request& r) {

    LOG_INFO() << r.peer() << ": reading body";

    std::vector<char> buffer;
    buffer.resize(m_buffer_size);

    co_await boost::asio::async_write(m_to, r.get_raw_buffer());

    std::size_t count = 0;
    do {
        count = co_await r.read_body({&buffer[0], m_buffer_size});
        auto raw_buffer = r.get_raw_buffer();
        co_await boost::asio::async_write(m_to, raw_buffer);
    } while (count == m_buffer_size);

    beast::http::response_parser<beast::http::empty_body> pr;
    pr.body_limit((std::numeric_limits<std::uint64_t>::max)());
    std::vector<char> b;
    auto buf = boost::asio::dynamic_buffer(b);

    co_await beast::http::async_read_header(m_to, buf, pr);

    auto m = pr.release();
    std::size_t len = 0;
    if (auto it = m.base().find("Content-Length"); it != m.base().end()) {
        len = std::stoul(it->value());
    }

    LOG_INFO() << r.peer() << ": read response of length " << len << " from downstream: " << m.result_int();

    co_return ep::http::response(std::move(m), std::make_unique<ep::http::raw_body>(m_to.socket(), std::move(b), len));
}

std::string forward_command::action_id() const {
    return "Forwarding";
}

} // namespace uh::cluster::proxy
