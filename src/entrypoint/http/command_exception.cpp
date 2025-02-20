#include "command_exception.h"

#include "common/telemetry/metrics.h"
#include "entrypoint/utils.h"

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

response make_response(const command_exception& e) {
    return error_response(e.m_status, e.m_code, e.m_reason);
}

ep::http::response error_response(ep::http::status status, std::string code,
                                  std::string reason) {
    boost::property_tree::ptree pt;
    pt.put("Error.Code", code);
    pt.put("Error.Message", reason);

    response res(status);
    res << pt;
    return res;
}

void throw_from_error(const error& e) {
    switch (*e) {
    case error::success:
        return;
    case error::bucket_already_exists:
        throw command_exception(status::conflict, "BucketAlreadyExists",
                                "The requested bucket name is not available.");
    case error::bucket_not_empty:
        throw command_exception(
            status::conflict, "BucketNotEmpty",
            "The bucket that you tried to delete is not empty.");
    case error::object_not_found:
        throw command_exception(status::not_found, "NoSuchKey",
                                "The specified key does not exist.");
    case error::bucket_not_found:
        throw command_exception(status::not_found, "NoSuchBucket",
                                "The specified bucket does not exist.");
    case error::storage_limit_exceeded:
        throw command_exception(status::insufficient_storage,
                                "InsufficientCapacity",
                                "Insufficient capacity.");
    case error::invalid_bucket_name:
        throw command_exception(
            status::bad_request, "InvalidBucketName",
            "The specified bucket name has invalid characters.");
    case error::internal_network_error:
        throw command_exception(status::internal_server_error, "InternalError",
                                "Downstream connection failed.");
    default:
        throw command_exception();
    }
}

} // namespace uh::cluster
