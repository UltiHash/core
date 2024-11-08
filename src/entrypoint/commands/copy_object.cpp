#include "copy_object.h"
#include "entrypoint/formats.h"
#include "entrypoint/utils.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/url/url.hpp>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

copy_object::copy_object(directory& dir, global_data_view& gdv, limits& limits)
    : m_directory(dir),
      m_gdv(gdv),
      m_limits(limits) {}

bool copy_object::can_handle(const request& req) {
    return req.method() == verb::put && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && !req.object_key().empty() &&
           req.header("x-amz-copy-source");
}

coro<response> copy_object::handle(request& req) {
    boost::urls::url url;
    url.set_encoded_path(*req.header("x-amz-copy-source"));

    auto [src_bucket, src_key] = extract_bucket_and_object(url);
    std::size_t freed = 0ull;
    object obj;

    {
        auto dir = co_await m_directory.get();
        auto lock = co_await dir.lock_object_shared(src_bucket, src_key);

        obj = co_await dir.get_object(src_bucket, src_key);

        if (auto ifmatch = req.header("x-amz-copy-source-if-match");
            ifmatch.value_or("") != obj.etag) {
            throw command_exception(
                status::precondition_failed, "PreconditionFailed",
                "At least one of the preconditions that you "
                "specified did not hold.");
        }

        m_limits.check_storage_size(obj.size);

        auto rejects = co_await m_gdv.link(req.context(), *obj.addr);
        if (!rejects.empty()) {
            LOG_ERROR() << req.peer()
                        << ": database contains object without references";
            throw command_exception(status::internal_server_error,
                                    "Data Corrupted", "found corrupted data");
        }

        obj.name = req.object_key();
        freed = co_await safe_put_object(req.context(), dir, m_gdv,
                                         req.bucket(), obj);
    }

    m_limits.free_storage_size(freed);

    boost::property_tree::ptree pt;
    put(pt, "CopyObjectResult.LastModified", iso8601_date(obj.last_modified));
    put(pt, "CopyObjectResult.ETag", obj.etag);

    response res;
    res << pt;

    co_return res;
}

std::string copy_object::action_id() const { return "s3:CopyObject"; }

} // namespace uh::cluster
