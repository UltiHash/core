#pragma once

#include <common/license/exp_backoff.h>

#include <boost/asio.hpp>
#include <common/types/common_types.h>

namespace uh::cluster::lic {

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

coro<std::string> fetch_response_body(boost::asio::io_context& io_context,
                                      const std::string& url,
                                      const std::string& username = "",
                                      const std::string& password = "");

/**
 * Exception handler for this fetch module.
 *
 * @return `true` means to "retry with exponential backoff"
 */
backoff_action fetch_exception_handler(const std::exception& e);

} // namespace uh::cluster::lic
