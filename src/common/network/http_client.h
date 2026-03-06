/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <boost/asio.hpp>
#include <common/network/async_http_client.h>
#include <common/telemetry/log.h>
#include <common/types/common_types.h>
#include <string>
#include <utility>

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

class http_client {
public:
    http_client(std::string&& username, std::string&& password,
                cpr::AuthMode auth_type)
        : m_async_client{std::move(username), std::move(password), auth_type} {}

    http_client(const std::string& username, const std::string& password,
                cpr::AuthMode auth_type)
        : m_async_client{username, password, auth_type} {}

    coro<std::string> co_get(std::string url) {
        auto r = co_await m_async_client.async_get( //
            std::move(url), boost::asio::use_awaitable);

        handle_status_code(r.error, r.status_code);
        co_return r.text;
    }

    coro<std::string> co_post(std::string url, cpr::Body body) {
        auto r = co_await m_async_client.async_post(
            std::move(url), std::move(body), boost::asio::use_awaitable);
        handle_status_code(r.error, r.status_code);
        co_return r.text;
    }

    coro<std::string> co_post(std::string url, cpr::Body body,
                              cpr::Header headers) {
        auto r = co_await m_async_client.async_post(
            std::move(url), std::move(body), std::move(headers),
            boost::asio::use_awaitable);
        handle_status_code(r.error, r.status_code);
        co_return r.text;
    }

private:
    static void handle_status_code(cpr::Error error, const long status_code) {

        if (error) {
            throw std::runtime_error(error.message);
        } else {
            LOG_DEBUG() << "Status code: " << status_code;
            if (status_code == 0) {
                throw std::runtime_error("HTTP request failed");
            } else if (status_code != 200) {
                auto ec = std::error_code(status_code, http_category());
                throw std::system_error(ec, "HTTP request failed");
            }
        }
    }

    async_http_client m_async_client;
};

} // namespace uh::cluster
