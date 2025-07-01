#pragma once

#include "common/utils/error.h"
#include "response.h"

namespace uh::cluster {

namespace beast = boost::beast;

class command_exception : public std::exception {
public:
    command_exception();
    command_exception(ep::http::status status, const std::string& code,
                      const std::string& reason);

    command_exception(const error::type& e);

    [[nodiscard]] const char* what() const noexcept override;
    ep::http::status get_status() const noexcept { return m_status; }

private:
    friend ep::http::response make_response(const command_exception&) noexcept;
    ep::http::status m_status = ep::http::status::internal_server_error;
    std::string m_code = "UnknownError";
    std::string m_reason = "Internal Server Error";
};

ep::http::response make_response(const command_exception& e) noexcept;
ep::http::response error_response(ep::http::status status, std::string code,
                                  std::string reason) noexcept;

} // namespace uh::cluster
