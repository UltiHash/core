#include "http_server.h"

#include "test_config.h"

#include <boost/filesystem.hpp>
#include <cpr/util.h>
#include <iostream>
#include <string>
#include <thread>

class http_server::impl {
public:
    impl(const std::string& user, const std::string& passwd)
        : m_server{},
          m_user{user},
          m_passwd{passwd} {

        m_server.set_logger([](const auto& req, const auto& res) {
            std::cout << "[http_server] " << req.method << " " << req.path
                      << " -> " << res.status << std::endl;
        });

        m_thread = std::jthread([this]() {
            m_port = m_server.bind_to_any_port("0.0.0.0");
            std::cout << "[http_server] Server running on http://localhost:"
                      << m_port << std::endl;
            m_server.listen_after_bind();
        });
        m_server.wait_until_ready();
    }

    ~impl() {
        m_server.stop();
        std::cout << "[INFO] Server stopped." << std::endl;
    }

    void set_get_handler(const std::string& path, GetHandler handler) {
        m_server.Get(path.c_str(), [this, handler](const httplib::Request& req,
                                                   httplib::Response& res) {
            if (!check_auth(req, res))
                return;
            handler(res);
        });
    }

    void set_post_handler(const std::string& path, PostHandler handler) {
        m_server.Post(path.c_str(), [this, handler](const httplib::Request& req,
                                                    httplib::Response& res) {
            if (!check_auth(req, res))
                return;
            handler(req, res);
        });
    }

    int get_port() const { return m_port; }

private:
    httplib::Server m_server;
    int m_port;
    std::string m_user;
    std::string m_passwd;
    std::jthread m_thread;

    bool check_auth(const httplib::Request& req, httplib::Response& res) {
        auto auth = req.get_header_value("Authorization");
        std::string expected_auth =
            "Basic " + httplib::detail::base64_encode(m_user + ":" + m_passwd);

        if (auth != expected_auth) {
            res.status = 401;
            res.set_header("WWW-Authenticate", "Basic realm=\"Secure Area\"");
            res.set_content("Unauthorized", "text/plain");
            return false;
        }
        return true;
    }
};

http_server::http_server(const std::string& user, const std::string& passwd)
    : pImpl(std::make_unique<impl>(user, passwd)) {}

http_server::~http_server() = default;

void http_server::set_get_handler(const std::string& path, GetHandler handler) {
    pImpl->set_get_handler(path, handler);
}

void http_server::set_post_handler(const std::string& path,
                                   PostHandler handler) {
    pImpl->set_post_handler(path, handler);
}

int http_server::get_port() const { return pImpl->get_port(); }
