#pragma once

#include <boost/asio.hpp>
#include <common/license/exp_backoff.h>
#include <common/types/common_types.h>

namespace uh::cluster {

class backend_client {
public:
    virtual ~backend_client() = default;
    virtual std::string get_license() const = 0;
    virtual void post_usage(std::string_view usage) = 0;
};

class default_backend_client : public backend_client {
public:
    struct config {
        std::string backend_host;
        std::string customer_id;
        std::string access_token;
        operator bool() const { return is_valid(); }

    private:
        bool is_valid() const {
            return !backend_host.empty() && //
                   !customer_id.empty() &&  //
                   !access_token.empty();
        }
    };

    explicit default_backend_client(const config& config)
        : m_backend_host(config.backend_host),
          m_customer_id(config.customer_id),
          m_access_token(config.access_token) {}

    std::string get_license() const;
    void post_usage(std::string_view usage);

private:
    const std::string& m_backend_host;
    const std::string& m_customer_id;
    const std::string& m_access_token;
};

class pseudo_backend_client : public backend_client {
public:
    pseudo_backend_client(std::string_view license_str)
        : m_license_str{license_str} {}

    std::string get_license() const { return m_license_str; }
    void post_usage(std::string_view usage) { (void)usage; }

private:
    const std::string m_license_str;
};

coro<std::string> fetch_response_body(boost::asio::io_context& io_context,
                                      const std::string& host,
                                      const std::string& path = "/",
                                      const std::string& username = "",
                                      const std::string& password = "");

} // namespace uh::cluster
