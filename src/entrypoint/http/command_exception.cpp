#include "command_exception.h"

#include "common/telemetry/log.h"
#include "common/telemetry/metrics.h"

namespace uh::cluster {

command_exception::command_exception()
    : command_exception(http::status::bad_request, "InternalServerError",
                        "internal server error") {}

command_exception::command_exception(http::status status,
                                     const std::string& code,
                                     const std::string& reason)
    : m_status(status),
      m_code(code),
      m_reason(reason) {
    metric<failure>::increase(1);
}

const char* command_exception::what() const noexcept {
    return m_reason.c_str();
}

http::response<http::string_body> make_response(const command_exception& e) {
    http::response<http::string_body> res{e.m_status, 11};
    if (e.m_status != http::status::not_found) {
        res.body() = "<Error>\n"
                     "<Code>" +
                     e.m_code +
                     "</Code>\n"
                     "<Message>" +
                     e.m_reason +
                     "</Message>\n"
                     "</Error>";
    }

    res.prepare_payload();

    LOG_DEBUG() << res.base();

    return res;
}

void throw_from_error(const error& e) {
    switch (*e) {
    case error::success:
        return;
    case error::bucket_already_exists:
        throw command_exception(http::status::conflict, "BucketAlreadyExists",
                                "bucket already exists");
    case error::bucket_not_empty:
        throw command_exception(http::status::conflict, "BucketNotEmpty",
                                "bucket is not empty");
    case error::object_not_found:
        throw command_exception(http::status::not_found, "NoSuchKey",
                                "object not found");
    case error::bucket_not_found:
        throw command_exception(http::status::not_found, "NoSuchBucket",
                                "bucket not found");
    case error::storage_limit_exceeded:
        throw command_exception(http::status::insufficient_storage,
                                "StorageLimitExceeded", "insufficient storage");
    case error::invalid_bucket_name:
        throw command_exception(http::status::bad_request, "InvalidBucketName",
                                "bucket name has invalid characters");
    default:
        throw command_exception();
    }
}

} // namespace uh::cluster
