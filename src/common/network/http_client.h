#pragma once

#include <boost/asio.hpp>
#include <common/network/async_http_client.h>
#include <common/types/common_types.h>
#include <string>

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

inline const http_error_category& http_category() {
    static http_error_category instance;
    return instance;
}

// class http_client {
// public:
//     http_client(std::string username, std::string password,
//                 cpr::AuthMode auth_type)
//         : m_async_client{std::move(username), std::move(password), auth_type}
//         {}
//
//     coro<std::string> co_post(const std::string& url, cpr::Body body) {
//         auto response = co_await m_async_client.async_post(
//             url, std::move(body), boost::asio::use_awaitable);
//         handle_response(response);
//         co_return response.text;
//     }
//
//     coro<std::string> co_get(const std::string& url) {
//         auto response = co_await m_async_client.async_get( //
//             url, boost::asio::use_awaitable);
//         handle_response(response);
//         co_return response.text;
//     }
//
// private:
//     static void handle_response(const cpr::Response& resp) {
//         std::error_code ec;
//         if (resp.status_code == 0) {
//             ec = std::make_error_code(std::errc::protocol_error);
//         } else if (resp.status_code != 200) {
//             ec = std::error_code(resp.status_code, http_category());
//         }
//
//         if (ec) {
//             throw std::system_error(ec, "HTTP request failed");
//         }
//     }
//
//     async_http_client m_async_client;
// };

} // namespace uh::cluster
