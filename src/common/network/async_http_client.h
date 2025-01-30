#pragma once

#include <boost/asio.hpp>
#include <boost/asio/async_result.hpp>
#include <common/coroutines/async_wrap.h>
#include <cpr/cpr.h>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace uh::cluster {

class async_http_client {
public:
    async_http_client(std::string username, std::string password,
                      cpr::AuthMode auth_type)
        : m_username(std::move(username)),
          m_password(std::move(password)),
          m_auth_type(auth_type) {}

    template <typename CompletionToken>
    auto async_get(const std::string& url, CompletionToken&& token) {
        return async_wrap<cpr::Response>(
            [this](const std::string& url, auto callback) {
                auto callback_ptr =
                    std::make_shared<decltype(callback)>(std::move(callback));
                cpr::GetCallback(
                    [callback_ptr] //
                    (cpr::Response resp) mutable {
                        (*callback_ptr)(std::move(resp));
                    },
                    cpr::Url{url},
                    cpr::Authentication{m_username, m_password, m_auth_type});
            },
            std::forward<CompletionToken>(token), url);
    }

    template <typename CompletionToken>
    auto async_post(const std::string& url, cpr::Body body,
                    CompletionToken&& token) {
        return async_wrap<cpr::Response>(
            [this](const std::string& url, cpr::Body body, auto callback) {
                auto callback_ptr =
                    std::make_shared<decltype(callback)>(std::move(callback));
                cpr::PostCallback(
                    [callback_ptr] //
                    (cpr::Response resp) mutable {
                        (*callback_ptr)(std::move(resp));
                    },
                    cpr::Url{url},
                    cpr::Authentication{m_username, m_password, m_auth_type},
                    std::move(body));
            },
            std::forward<CompletionToken>(token), url, std::move(body));
    }

private:
    std::string m_username;
    std::string m_password;
    cpr::AuthMode m_auth_type;
};

} // namespace uh::cluster
