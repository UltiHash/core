#pragma once

#include <boost/asio.hpp>
#include <common/license/exp_backoff.h>
#include <common/network/http_client.h>
#include <common/types/common_types.h>
#include <magic_enum/magic_enum.hpp>

namespace uh::cluster {

class backend_client {
public:
    virtual ~backend_client() = default;
    virtual coro<std::string> get_license() = 0;
    virtual coro<std::string> post_usage(std::string usage) = 0;
};

class default_backend_client : public backend_client {
public:
    enum class type : uint8_t { http, https };
    struct config {
        std::string backend_host{};
        std::string customer_id{};
        std::string access_token{};
        operator bool() const { return is_valid(); }

    private:
        bool is_valid() const {
            return !backend_host.empty() && //
                   !customer_id.empty() &&  //
                   !access_token.empty();
        }
    };

    default_backend_client(auto&& backend_host, auto&& customer_id,
                           auto&& access_token, type type = type::https)
        : m_backend_host(std::forward<decltype(backend_host)>(backend_host)),
          m_backend_type{type},
          m_http_client{std::forward<decltype(customer_id)>(customer_id),
                        std::forward<decltype(access_token)>(access_token),
                        cpr::AuthMode::BASIC} {}

    coro<std::string> get_license() {
        auto url = std::string(magic_enum::enum_name(m_backend_type)) + "://" +
                   m_backend_host + "/v1/license";
        LOG_DEBUG() << "Fetching license from url: " << url;
        co_return co_await m_http_client.co_get(url);
    }

    coro<std::string> post_usage(std::string usage) {
        auto url = std::string(magic_enum::enum_name(m_backend_type)) + "://" +
                   m_backend_host + "/v1/usage";
        LOG_DEBUG() << "Posting usage to url: " << url
                    << "with usage: " << usage;

        cpr::Header hdr;
        hdr["Content-Type"] = "application/json";
        co_return co_await m_http_client.co_post(url, std::move(usage),
                                                 std::move(hdr));
    }

private:
    std::string m_backend_host;
    type m_backend_type;
    http_client m_http_client;
};

class pseudo_backend_client : public backend_client {
public:
    pseudo_backend_client(auto&& license_str)
        : m_license_str{std::forward<decltype(license_str)>(license_str)} {}

    coro<std::string> get_license() { co_return m_license_str; }
    coro<std::string> post_usage(std::string usage) {
        co_return std::move(usage);
    }

private:
    const std::string m_license_str;
};

coro<std::string> fetch_response_body(boost::asio::io_context& io_context,
                                      const std::string& host,
                                      const std::string& path = "/",
                                      const std::string& username = "",
                                      const std::string& password = "");

} // namespace uh::cluster
