#include "command_exception.h"

#include <common/telemetry/log.h>
#include <common/telemetry/metrics.h>
#include <entrypoint/utils.h>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

command_exception::command_exception()
    : command_exception(status::internal_server_error, "InternalError",
                        "Internal server error.") {}

command_exception::command_exception(status status, const std::string& code,
                                     const std::string& reason)
    : m_status(status),
      m_code(code),
      m_reason(reason) {
    metric<failure>::increase(1);
}

const char* command_exception::what() const noexcept {
    return m_reason.c_str();
}

response make_response(const command_exception& e) noexcept {
    return error_response(e.m_status, e.m_code, e.m_reason);
}

ep::http::response error_response(ep::http::status status, std::string code,
                                  std::string reason) noexcept {
    boost::property_tree::ptree pt;
    pt.put("Error.Code", code);
    pt.put("Error.Message", reason);

    response res(status);
    res << pt;
    return res;
}

command_exception::command_exception(const error::type& e) {
    switch (e) {
    case error::success:
        LOG_FATAL() << "Unexpected success error code in command_exception";
        throw;
    case error::bucket_already_exists:
        m_status = status::conflict;
        m_code = "BucketAlreadyExists";
        m_reason = "The requested bucket name is not available.";
        break;
    case error::bucket_not_empty:
        m_status = status::conflict;
        m_code = "BucketNotEmpty";
        m_reason = "The bucket that you tried to delete is not empty.";
        break;
    case error::object_not_found:
        m_status = status::not_found;
        m_code = "NoSuchKey";
        m_reason = "The specified key does not exist.";
        break;
    case error::bucket_not_found:
        m_status = status::not_found;
        m_code = "NoSuchBucket";
        m_reason = "The specified bucket does not exist.";
        break;
    case error::storage_limit_exceeded:
        m_status = status::insufficient_storage;
        m_code = "InsufficientCapacity";
        m_reason = "Insufficient capacity.";
        break;
    case error::invalid_bucket_name:
        m_status = status::bad_request;
        m_code = "InvalidBucketName";
        m_reason = "The specified bucket name has invalid characters.";
        break;
    case error::internal_network_error:
        m_status = status::internal_server_error;
        m_code = "InternalError";
        m_reason = "Downstream connection failed.";
        break;
    case error::busy:
        m_status = status::service_unavailable;
        m_code = "SlowDown";
        m_reason = "Please reduce your request rate.";
        break;
    case error::service_unavailable:
        m_status = status::service_unavailable;
        m_code = "ServiceUnavailable";
        m_reason = "Service is unable to handle request.";
        break;
    default:
        m_status = status::internal_server_error;
        m_code = "UnknownError";
        m_reason = "An unknown error occurred.";
        break;
    }
}

} // namespace uh::cluster
