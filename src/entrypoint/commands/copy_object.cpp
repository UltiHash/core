#include "copy_object.h"
#include "entrypoint/formats.h"
#include "entrypoint/utils.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/url/url.hpp>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

copy_object::copy_object(directory& dir, global_data_view& gdv)
    : m_directory(dir),
      m_gdv(gdv) {}

bool copy_object::can_handle(const request& req) {
    return req.method() == verb::put && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && !req.object_key().empty() &&
           req.header("x-amz-copy-source");
}

coro<response> copy_object::handle(request& req) {
    boost::urls::url url;
    url.set_encoded_path(*req.header("x-amz-copy-source"));

    auto [src_bucket, src_key] = extract_bucket_and_object(url);
    auto dir = co_await m_directory.get();
    object obj;

    auto lock = co_await dir.lock_object_shared(src_bucket, src_key);
    obj = co_await dir.get_object(src_bucket, src_key);

    if (auto ifmatch = req.header("x-amz-copy-source-if-match");
        ifmatch.value_or("") != obj.etag) {
        throw command_exception(status::precondition_failed,
                                "PreconditionFailed",
                                "At least one of the preconditions that you "
                                "specified did not hold.");
    }

    auto rejects = co_await m_gdv.link(req.context(), obj.addr.value());
    if (!rejects.empty()) {
        co_await m_gdv.unlink(req.context(), *obj.addr);
        LOG_ERROR() << req.peer()
                    << ": database contains object without references";
        throw command_exception(status::internal_server_error, "Data Corrupted",
                                "found corrupted data");
    }

    auto obj_lock = co_await dir.lock_object(req.bucket(), req.object_key());
    try {
        auto old_obj = co_await dir.get_object(req.bucket(), req.object_key());
        co_await m_gdv.unlink(req.context(), *old_obj.addr);
    } catch (const std::exception& e) {
        LOG_WARN() << req.peer()
                   << ": error deleting original object data: " << e.what();
    }

    obj.name = req.object_key();
    std::optional<std::exception_ptr> error;

    try {
        co_await dir.put_object(req.bucket(), obj);
    } catch (...) {
        error = std::current_exception();
    }

    if (error) {
        co_await m_gdv.unlink(req.context(), *obj.addr);
        throw *error;
    }

    boost::property_tree::ptree pt;
    put(pt, "CopyObjectResult.LastModified", iso8601_date(obj.last_modified));
    put(pt, "CopyObjectResult.ETag", obj.etag);

    response res;
    res << pt;

    co_return res;
}

std::string copy_object::action_id() const { return "s3:CopyObject"; }

} // namespace uh::cluster
