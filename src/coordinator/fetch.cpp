#include "fetch.h"

#include "common/telemetry/log.h"
#include "common/utils/error.h"

using boost::asio::use_awaitable;
using boost::asio::ip::tcp;

namespace uh::cluster {

coro<std::string> fetch_response_body(boost::asio::io_context& io_context,
                                      const std::string& url) {

    auto const pos = url.find('/');
    std::string host = url.substr(0, pos);
    std::string path = (pos == std::string::npos) ? "/" : url.substr(pos);

    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = co_await resolver.async_resolve(
        host, "http", boost::asio::use_awaitable);

    boost::asio::ip::tcp::socket socket(io_context);
    co_await boost::asio::async_connect(socket, endpoints,
                                        boost::asio::use_awaitable);

    std::string request = "GET " + path +
                          " HTTP/1.1\r\n"
                          "Host: " +
                          host + "\r\n\r\n";
    co_await boost::asio::async_write(socket, boost::asio::buffer(request),
                                      boost::asio::use_awaitable);

    // char response[1024];
    // std::size_t n = co_await socket.async_read_some(
    //     boost::asio::buffer(response), boost::asio::use_awaitable);
    // co_return std::string_view(response, response + n);

    boost::asio::streambuf response_buf;
    std::size_t n = co_await boost::asio::async_read_until(
        socket, response_buf, "\r\n\r\n", use_awaitable);
    if (n == 0) {
        LOG_ERROR() << "No data received";
        throw error_exception(error::internal_network_error);
    }

    std::istream response_stream(&response_buf);
    std::string http_version;
    unsigned int status_code;
    std::string status_message;

    response_stream >> http_version >> status_code;
    std::getline(response_stream, status_message);

    if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
        LOG_ERROR() << "Invalid response";
        throw error_exception(error::internal_network_error);
    }

    // Skip the headers
    std::string header;
    std::size_t content_length = 0;
    while (std::getline(response_stream, header) && header != "\r") {
        if (header.find("Content-Length:") != std::string::npos) {
            content_length = std::stoul(header.substr(16));
        }
    }

    // Read the response body
    std::string response_body;
    if (response_buf.size() > 0) {
        response_body.assign(std::istreambuf_iterator<char>(response_stream),
                             std::istreambuf_iterator<char>());
    }

    // Read remaining data based on content length
    while (response_body.size() < content_length) {
        char buf[512];
        std::size_t bytes_transferred = co_await socket.async_read_some(
            boost::asio::buffer(buf), use_awaitable);
        if (bytes_transferred == 0)
            break;
        response_body.append(buf, buf + bytes_transferred);
    }

    co_return response_body;
}

} // namespace uh::cluster
