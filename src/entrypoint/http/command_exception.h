#ifndef CORE_ENTRYPOINT_HTTP_COMMAND_EXCEPTION_H
#define CORE_ENTRYPOINT_HTTP_COMMAND_EXCEPTION_H

#include "common/utils/error.h"
#include "response.h"

namespace uh::cluster {

namespace beast = boost::beast;

class command_exception : public std::exception {
public:
    command_exception();
    command_exception(ep::http::status status, const std::string& code,
                      const std::string& reason);

    [[nodiscard]] const char* what() const noexcept override;

private:
    friend ep::http::response make_response(const command_exception&);
    ep::http::status m_status = ep::http::status::internal_server_error;
    std::string m_code = "UnknownError";
    std::string m_reason = "Internal Server Error";
};

ep::http::response make_response(const command_exception& e);

void throw_from_error(const error& e);

} // namespace uh::cluster

#endif
