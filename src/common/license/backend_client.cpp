#include <common/license/backend_client.h>

#include <common/telemetry/log.h>
#include <common/utils/error.h>
#include <common/utils/strings.h>

#include <boost/asio/ssl.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <cpr/cpr.h>

using boost::asio::use_awaitable;
using boost::asio::ip::tcp;

namespace uh::cluster {

std::string default_backend_client::get_license() const {

    auto resp = cpr::Get(cpr::Url{"https://" + m_backend_host + "/v1/license"},
                         cpr::Authentication{m_customer_id, m_access_token,
                                             cpr::AuthMode::BASIC});
    if (resp.status_code == 0) {
        throw std::runtime_error("Invalid response");
    }
    if (resp.status_code != 200) {
        std::error_code ec(resp.status_code, http_category());
        throw std::system_error(ec, "HTTP request failed");
    }
    return resp.text;
}

void default_backend_client::post_usage(std::string_view usage) {
    // Reference https://docs.libcpr.org/introduction.html#post-requests
    auto resp = cpr::Post(cpr::Url{"https://" + m_backend_host + "/v1/usage"},
                          cpr::Body{usage.data()},
                          cpr::Header{{"Content-Type", "text/plain"}},
                          cpr::Authentication{m_customer_id, m_access_token,
                                              cpr::AuthMode::BASIC});
    if (resp.status_code == 0) {
        throw std::runtime_error("Invalid response");
    }
    if (resp.status_code != 200) {
        std::error_code ec(resp.status_code, http_category());
        throw std::system_error(ec, "HTTP request failed");
    }
}

} // namespace uh::cluster
