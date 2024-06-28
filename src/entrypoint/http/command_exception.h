#ifndef CORE_ENTRYPOINT_HTTP_COMMAND_EXCEPTION_H
#define CORE_ENTRYPOINT_HTTP_COMMAND_EXCEPTION_H

#include "common/utils/error.h"
#include "http_response.h"
#include <boost/beast/core.hpp>

namespace uh::cluster {

namespace http = boost::beast::http; // from <boost/beast/http.hpp>

class command_exception : public std::exception {
public:
    command_exception();
    command_exception(http::status status, const std::string& code,
                      const std::string& reason);

    [[nodiscard]] const char* what() const noexcept override;

private:
    friend http_response make_response(const command_exception&);
    http::status m_status = http::status::internal_server_error;
    std::string m_code = "UnknownError";
    std::string m_reason = "Internal Server Error";
};

http_response make_response(const command_exception& e);

void throw_from_error(const error& e);

} // namespace uh::cluster

#endif
