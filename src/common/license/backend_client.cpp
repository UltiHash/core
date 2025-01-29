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

std::string default_backend_client::get_license() {

    cpr::Response r =
        cpr::Get(cpr::Url{"https://" + m_backend_host + "/v1/license"},
                 cpr::Authentication{m_customer_id, m_access_token,
                                     cpr::AuthMode::BASIC});
    return r.text;
}

void default_backend_client::post_usage(std::string_view usage) {
    // Reference https://docs.libcpr.org/introduction.html#post-requests
    cpr::Response r = cpr::Post(
        cpr::Url{"https://" + m_backend_host + "/v1/usage"},
        cpr::Body{usage.data()}, cpr::Header{{"Content-Type", "text/plain"}},
        cpr::Authentication{m_customer_id, m_access_token,
                            cpr::AuthMode::BASIC});
}

} // namespace uh::cluster
