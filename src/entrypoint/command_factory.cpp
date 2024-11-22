#include "command_factory.h"

#include "commands/abort_multipart.h"
#include "commands/complete_multipart.h"
#include "commands/copy_object.h"
#include "commands/create_bucket.h"
#include "commands/delete_bucket.h"
#include "commands/delete_bucket_policy.h"
#include "commands/delete_object.h"
#include "commands/delete_objects.h"
#include "commands/get_bucket_policy.h"
#include "commands/get_metrics.h"
#include "commands/get_object.h"
#include "commands/head_bucket.h"
#include "commands/head_object.h"
#include "commands/init_multipart.h"
#include "commands/list_buckets.h"
#include "commands/list_multipart.h"
#include "commands/list_objects.h"
#include "commands/list_objects_v2.h"
#include "commands/multipart.h"
#include "commands/put_bucket_policy.h"
#include "commands/put_object.h"

#include "commands/iam/create_access_key.h"
#include "commands/iam/create_user.h"
#include "commands/iam/delete_access_key.h"
#include "commands/iam/delete_user.h"
#include "commands/iam/delete_user_policy.h"
#include "commands/iam/get_user_policy.h"
#include "commands/iam/list_user_policies.h"
#include "commands/iam/put_user_policy.h"

namespace uh::cluster {

coro<std::unique_ptr<command>>
command_factory::action_command(ep::http::request& req) {
    auto length = std::stoull(req.header("content-length").value_or("0"));
    if (length == 0) {
        throw command_exception(ep::http::status::bad_request,
                                "CommandNotFound", "no such command found");
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

    throw command_exception(ep::http::status::bad_request, "CommandNotFound",
                            "no such command found");
}

coro<std::unique_ptr<command>> command_factory::create(ep::http::request& req) {
    if (req.method() == ep::http::verb::post && req.path() == "/") {
        co_return co_await action_command(req);
    }

    if (get_object::can_handle(req)) {
        co_return std::make_unique<get_object>(m_directory, m_gdv);
    }
    if (put_object::can_handle(req)) {
        co_return std::make_unique<put_object>(
            m_ioc, m_config, m_limits, m_directory, m_gdv, m_dedupe_services);
    }
    if (multipart::can_handle(req)) {
        co_return std::make_unique<multipart>(m_dedupe_services, m_gdv,
                                              m_uploads);
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

    throw command_exception(ep::http::status::bad_request, "CommandNotFound",
                            "no such command found");
}

limits& command_factory::get_limits() const { return m_limits; }

directory& command_factory::get_directory() const { return m_directory; }

} // namespace uh::cluster
