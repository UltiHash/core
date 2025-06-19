#include "command_factory.h"

#include <entrypoint/commands/s3/abort_multipart.h>
#include <entrypoint/commands/s3/complete_multipart.h>
#include <entrypoint/commands/s3/copy_object.h>
#include <entrypoint/commands/s3/create_bucket.h>
#include <entrypoint/commands/s3/delete_bucket.h>
#include <entrypoint/commands/s3/delete_bucket_cors.h>
#include <entrypoint/commands/s3/delete_bucket_policy.h>
#include <entrypoint/commands/s3/delete_object.h>
#include <entrypoint/commands/s3/delete_objects.h>
#include <entrypoint/commands/s3/get_bucket_cors.h>
#include <entrypoint/commands/s3/get_bucket_policy.h>
#include <entrypoint/commands/s3/get_bucket_versioning.h>
#include <entrypoint/commands/s3/get_object.h>
#include <entrypoint/commands/s3/head_bucket.h>
#include <entrypoint/commands/s3/head_object.h>
#include <entrypoint/commands/s3/init_multipart.h>
#include <entrypoint/commands/s3/list_buckets.h>
#include <entrypoint/commands/s3/list_multipart.h>
#include <entrypoint/commands/s3/list_objects.h>
#include <entrypoint/commands/s3/list_objects_v2.h>
#include <entrypoint/commands/s3/multipart.h>
#include <entrypoint/commands/s3/put_bucket_cors.h>
#include <entrypoint/commands/s3/put_bucket_policy.h>
#include <entrypoint/commands/s3/put_bucket_versioning.h>
#include <entrypoint/commands/s3/put_object.h>

#include <entrypoint/commands/uh/get_license_info.h>
#include <entrypoint/commands/uh/get_metrics.h>
#include <entrypoint/commands/uh/get_ready.h>

#include <entrypoint/commands/iam/create_access_key.h>
#include <entrypoint/commands/iam/create_user.h>
#include <entrypoint/commands/iam/delete_access_key.h>
#include <entrypoint/commands/iam/delete_user.h>
#include <entrypoint/commands/iam/delete_user_policy.h>
#include <entrypoint/commands/iam/get_user_policy.h>
#include <entrypoint/commands/iam/list_user_policies.h>
#include <entrypoint/commands/iam/put_user_policy.h>

namespace uh::cluster {

coro<std::unique_ptr<command>>
command_factory::action_command(ep::http::request& req) {
    auto length = std::stoull(req.header("content-length").value_or("0"));
    if (length == 0) {
        throw command_exception(ep::http::status::bad_request, "InvalidURI",
                                "The specified URI couldn't be parsed.");
    }

    if (length > MAX_POST_QUERY_LENGTH) {
        throw command_exception(ep::http::status::bad_request,
                                "MaxMessageLengthExceeded",
                                "Your request was too large.");
    }

    std::string post_query(length, '\0');
    auto size = co_await req.read_body({&post_query[0], length});
    post_query.resize(size);

    req.set_query_params(post_query);

    if (ep::iam::create_user::can_handle(req)) {
        co_return std::make_unique<ep::iam::create_user>(m_users);
    }

    if (ep::iam::delete_user::can_handle(req)) {
        co_return std::make_unique<ep::iam::delete_user>(m_users);
    }

    if (ep::iam::create_access_key::can_handle(req)) {
        co_return std::make_unique<ep::iam::create_access_key>(m_users);
    }

    if (ep::iam::delete_access_key::can_handle(req)) {
        co_return std::make_unique<ep::iam::delete_access_key>(m_users);
    }

    if (ep::iam::put_user_policy::can_handle(req)) {
        co_return std::make_unique<ep::iam::put_user_policy>(m_users);
    }

    if (ep::iam::list_user_policies::can_handle(req)) {
        co_return std::make_unique<ep::iam::list_user_policies>(m_users);
    }

    if (ep::iam::get_user_policy::can_handle(req)) {
        co_return std::make_unique<ep::iam::get_user_policy>(m_users);
    }

    if (ep::iam::delete_user_policy::can_handle(req)) {
        co_return std::make_unique<ep::iam::delete_user_policy>(m_users);
    }

    throw command_exception(ep::http::status::bad_request, "InvalidURI",
                            "The specified URI couldn't be parsed.");
}

coro<std::unique_ptr<command>> command_factory::create(ep::http::request& req) {
    if (req.method() == ep::http::verb::post && req.path() == "/") {
        co_return co_await action_command(req);
    }

    if (get_object::can_handle(req)) {
        co_return std::make_unique<get_object>(m_directory, m_gdv);
    }
    if (put_object::can_handle(req)) {
        co_return std::make_unique<put_object>(m_ioc, m_config, m_limits,
                                               m_directory, m_gdv, m_dedupe);
    }
    if (multipart::can_handle(req)) {
        co_return std::make_unique<multipart>(m_dedupe, m_gdv, m_uploads);
    }
    if (init_multipart::can_handle(req)) {
        co_return std::make_unique<init_multipart>(m_directory, m_uploads);
    }
    if (complete_multipart::can_handle(req)) {
        co_return std::make_unique<complete_multipart>(m_directory, m_gdv,
                                                       m_uploads, m_limits);
    }
    if (list_objects_v2::can_handle(req)) {
        co_return std::make_unique<list_objects_v2>(m_directory);
    }
    if (list_objects::can_handle(req)) {
        co_return std::make_unique<list_objects>(m_directory);
    }
    if (list_buckets::can_handle(req)) {
        co_return std::make_unique<list_buckets>(m_directory);
    }
    if (head_object::can_handle(req)) {
        co_return std::make_unique<head_object>(m_directory);
    }
    if (head_bucket::can_handle(req)) {
        co_return std::make_unique<head_bucket>(m_directory);
    }
    if (create_bucket::can_handle(req)) {
        co_return std::make_unique<create_bucket>(m_directory);
    }
    if (copy_object::can_handle(req)) {
        co_return std::make_unique<copy_object>(m_directory, m_gdv, m_limits);
    }
    if (list_multipart::can_handle(req)) {
        co_return std::make_unique<list_multipart>(m_uploads);
    }
    if (get_license_info::can_handle(req)) {
        co_return std::make_unique<get_license_info>(m_license_watcher);
    }
    if (get_metrics::can_handle(req)) {
        co_return std::make_unique<get_metrics>(m_directory, m_gdv);
    }
    if (delete_object::can_handle(req)) {
        co_return std::make_unique<delete_object>(m_directory, m_gdv, m_limits);
    }
    if (delete_objects::can_handle(req)) {
        co_return std::make_unique<delete_objects>(m_directory, m_gdv,
                                                   m_limits);
    }
    if (delete_bucket::can_handle(req)) {
        co_return std::make_unique<delete_bucket>(m_directory);
    }
    if (abort_multipart::can_handle(req)) {
        co_return std::make_unique<abort_multipart>(m_uploads, m_gdv);
    }
    if (get_bucket_policy::can_handle(req)) {
        co_return std::make_unique<get_bucket_policy>(m_directory);
    }
    if (put_bucket_policy::can_handle(req)) {
        co_return std::make_unique<put_bucket_policy>(m_directory);
    }
    if (delete_bucket_policy::can_handle(req)) {
        co_return std::make_unique<delete_bucket_policy>(m_directory);
    }
    if (get_ready::can_handle(req)) {
        co_return std::make_unique<get_ready>(m_directory, m_gdv);
    }

    if (get_bucket_cors::can_handle(req)) {
        co_return std::make_unique<get_bucket_cors>(m_directory);
    }
    if (delete_bucket_cors::can_handle(req)) {
        co_return std::make_unique<delete_bucket_cors>(m_directory);
    }
    if (put_bucket_cors::can_handle(req)) {
        co_return std::make_unique<put_bucket_cors>(m_directory);
    }

    if (get_bucket_versioning::can_handle(req)) {
        co_return std::make_unique<get_bucket_versioning>(m_directory);
    }
    if (put_bucket_versioning::can_handle(req)) {
        co_return std::make_unique<put_bucket_versioning>(m_directory);
    }

    throw command_exception(ep::http::status::bad_request, "InvalidURI",
                            "The specified URI couldn't be parsed.");
}

limits& command_factory::get_limits() const { return m_limits; }

directory& command_factory::get_directory() const { return m_directory; }

} // namespace uh::cluster
