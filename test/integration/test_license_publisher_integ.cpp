#define BOOST_TEST_MODULE "license publisher tests"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/test/unit_test.hpp>
#include <common/utils/strings.h>
#include <memory>
#include <string>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

class http_server {
public:
    http_server(net::io_context& ioc, ssl::context& ssl_ctx,
                tcp::endpoint endpoint, const std::string& username,
                const std::string& password)
        : m_acceptor(ioc),
          m_ssl_ctx(ssl_ctx),
          m_username(username),
          m_password(password) {
        m_acceptor.open(endpoint.protocol());
        m_acceptor.set_option(net::socket_base::reuse_address(true));
        m_acceptor.bind(endpoint);
        m_acceptor.listen(net::socket_base::max_listen_connections);

        do_accept();
    }

    void set_response(http::status status, std::string body) {
        m_response_status = status;
        m_response_body = std::move(body);
    }

private:
    void do_accept() {
        m_acceptor.async_accept([this](beast::error_code, tcp::socket socket) {
            auto ssl_sock = std::make_shared<beast::ssl_stream<tcp::socket>>(
                std::move(socket), m_ssl_ctx);
            ssl_sock->async_handshake(ssl::stream_base::server,
                                      [this, ssl_sock](beast::error_code) {
                                          handle_request(ssl_sock);
                                      });
            do_accept();
        });
    }

    void
    handle_request(std::shared_ptr<beast::ssl_stream<tcp::socket>> ssl_sock) {
        auto req = std::make_shared<http::request<http::string_body>>();
        beast::flat_buffer buffer;

        http::async_read(*ssl_sock, buffer, *req,
                         [this, ssl_sock, req](beast::error_code, std::size_t) {
                             if (validate_auth(*req)) {
                                 send_response(ssl_sock, req);
                             } else {
                                 send_unauthorized(ssl_sock, req);
                             }
                         });
    }

    bool validate_auth(const http::request<http::string_body>& req) {
        auto auth_header = req.find(http::field::authorization);
        if (auth_header == req.end()) {
            return false;
        }

        std::string auth = auth_header->value();
        if (auth.substr(0, 6) != "Basic ") {
            return false;
        }

        std::string encoded = auth.substr(6);

        std::vector<char> _decoded = uh::cluster::base64_decode(encoded);
        auto decoded = std::string(_decoded.begin(), _decoded.end());
        std::vector<std::string> parts;
        boost::split(parts, decoded, boost::is_any_of(":"));

        if (parts.size() != 2 || parts[0] != m_username ||
            parts[1] != m_password) {
            return false;
        }

        return true;
    }

    void
    send_unauthorized(std::shared_ptr<beast::ssl_stream<tcp::socket>> ssl_sock,
                      std::shared_ptr<http::request<http::string_body>> req) {
        auto res = std::make_shared<http::response<http::string_body>>();
        res->version(req->version());
        res->result(http::status::unauthorized);
        res->set(http::field::www_authenticate, "Basic realm=\"Test Server\"");
        res->set(http::field::content_type, "text/plain");
        res->body() = "Unauthorized";
        res->prepare_payload();

        http::async_write(
            *ssl_sock, *res, [ssl_sock](beast::error_code, std::size_t) {
                ssl_sock->async_shutdown([](beast::error_code) {});
            });
    }

    void send_response(std::shared_ptr<beast::ssl_stream<tcp::socket>> ssl_sock,
                       std::shared_ptr<http::request<http::string_body>> req) {
        auto res = std::make_shared<http::response<http::string_body>>();
        res->version(req->version());
        res->result(m_response_status);
        res->set(http::field::server, "Test Server");
        res->set(http::field::content_type, "text/plain");
        res->body() = m_response_body;
        res->prepare_payload();

        http::async_write(
            *ssl_sock, *res, [ssl_sock](beast::error_code, std::size_t) {
                ssl_sock->async_shutdown([](beast::error_code) {});
            });
    }

    tcp::acceptor m_acceptor;
    ssl::context& m_ssl_ctx;
    std::string m_username;
    std::string m_password;

    http::status m_response_status = http::status::ok;
    std::string m_response_body = "Hello, World!";
};

struct http_server_fixture {
    http_server_fixture()
        : m_ctx(),
          m_ssl_ctx(ssl::context::tlsv12),
          m_server(m_ctx, m_ssl_ctx, tcp::endpoint{tcp::v4(), 8080}, "user",
                   "pass") {
        m_ssl_ctx.use_certificate_chain_file("server.crt");
        m_ssl_ctx.use_private_key_file("server.key", ssl::context::pem);

        m_server_thread = std::thread([this] { m_ctx.run(); });
    }

    ~http_server_fixture() {
        m_ctx.stop();
        if (m_server_thread.joinable())
            m_server_thread.join();
    }

    void set_response(http::status status, std::string body) {
        m_server.set_response(status, std::move(body));
    }

private:
    net::io_context m_ctx;
    ssl::context m_ssl_ctx;
    http_server m_server;
    std::thread m_server_thread;
};

BOOST_FIXTURE_TEST_SUITE(http_server_test, http_server_fixture)

BOOST_AUTO_TEST_CASE(test_success_response) {
    set_response(http::status::ok, "Success");
}

BOOST_AUTO_TEST_CASE(test_error_response) {
    set_response(http::status::internal_server_error, "Error");
}

BOOST_AUTO_TEST_SUITE_END()
