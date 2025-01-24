#include "fetch.h"

#include <common/telemetry/log.h>
#include <common/utils/error.h>
#include <common/utils/strings.h>

#include <boost/asio/ssl.hpp>
#include <boost/beast/core/detail/base64.hpp>

using boost::asio::use_awaitable;
using boost::asio::ip::tcp;

namespace uh::cluster {

class http_error_category : public std::error_category {
public:
    const char* name() const noexcept override { return "http"; }

    std::string message(int ev) const override {
        switch (ev) {
        case 200:
            return "OK";
        case 202:
            return "Accepted";
        case 401:
            return "Unauthorized";
        case 429:
            return "Overloaded";
        }
        if (400 <= ev && ev < 500)
            return "Bad Request";
        if (500 <= ev && ev < 600)
            return "Error on Backend";

        return "Unknown";
    }
};

const http_error_category& http_category() {
    static http_error_category instance;
    return instance;
}

coro<std::string> fetch_response_body(boost::asio::io_context& io_context,
                                      const std::string& url,
                                      const std::string& username,
                                      const std::string& password) {

    auto const pos = url.find('/');
    std::string host = url.substr(0, pos);
    std::string path = (pos == std::string::npos) ? "/" : url.substr(pos);

    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = co_await resolver.async_resolve(
        host, "https", boost::asio::use_awaitable);

    boost::asio::ssl::context ssl_context(boost::asio::ssl::context::sslv23);
    boost::asio::ssl::stream<tcp::socket> socket(io_context, ssl_context);

    co_await boost::asio::async_connect(socket.lowest_layer(), endpoints,
                                        boost::asio::use_awaitable);

    socket.handshake(boost::asio::ssl::stream_base::client);

    std::string request = "GET " + path +
                          " HTTP/1.1\r\n"
                          "Host: " +
                          host + "\r\n";
    if (!username.empty() && !password.empty()) {
        std::string auth = username + ":" + password;
        std::string encoded_auth{base64_encode(auth).data()};
        request += "Authorization: Basic " + encoded_auth + "\r\n";
    }
    request += "\r\n";

    co_await boost::asio::async_write(socket, boost::asio::buffer(request),
                                      boost::asio::use_awaitable);

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
        throw std::runtime_error("Invalid response");
    } else if (status_code != 200) {
        std::error_code ec(status_code, http_category());
        throw std::system_error(ec, "HTTP request failed");
    }

    std::string header;
    std::size_t content_length = 0;
    while (std::getline(response_stream, header) && header != "\r") {
        if (header.find("Content-Length:") != std::string::npos) {
            content_length = std::stoul(header.substr(16));
        }
    }

    std::string response_body;
    if (response_buf.size() > 0) {
        response_body.assign(std::istreambuf_iterator<char>(response_stream),
                             std::istreambuf_iterator<char>());
    }

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

coro<std::string> exponential_backoff(boost::asio::io_context& io_context,
                                      std::function<coro<std::string>()> task) {
    std::string rv;
    int max_retries = 5;
    int retry_count = 0;
    int delay = 1; // initial delay in seconds

    while (retry_count < max_retries) {
        bool success = false;
        try {
            // TODO: What should we do when the license server is down
            // for one hour? Should I clear license information on etcd?
            rv = co_await task();
            success = true;

        } catch (const std::system_error& e) {
            std::cout << "Caught system_error: " << e.what() << std::endl;
            std::cout << "Error code: " << e.code() << std::endl;
            std::cout << "Error message: " << e.code().message() << std::endl;
            // TODO: Implement exponential backoff.
            retry_count++;
        } catch (const std::runtime_error& e) {
            std::cout << "Caught runtime_error: " << e.what() << std::endl;
        }

        if (success) {
            break;
        }

        if (retry_count < max_retries) {
            boost::asio::steady_timer timer(io_context,
                                            std::chrono::seconds(delay));
            co_await timer.async_wait(boost::asio::use_awaitable);
            delay *= 2; // exponential backoff
        } else {
            std::cerr << "Max retries reached. Exiting." << std::endl;
        }
    }
    co_return rv;
}

} // namespace uh::cluster
