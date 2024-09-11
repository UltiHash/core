#include "command_factory.h"

#include "commands/abort_multipart.h"
#include "commands/complete_multipart.h"
#include "commands/copy_object.h"
#include "commands/create_bucket.h"
#include "commands/delete_bucket.h"
#include "commands/delete_object.h"
#include "commands/delete_objects.h"
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
#include "commands/put_object.h"

namespace uh::cluster {

std::unique_ptr<command>
command_factory::create(const ep::http::request& req) const {
    if (get_object::can_handle(req)) {
        return std::make_unique<get_object>(m_directory, m_gdv);
    }
    if (put_object::can_handle(req)) {
        return std::make_unique<put_object>(m_ioc, m_config, m_limits,
                                            m_directory, m_dedupe_services);
    }
    if (multipart::can_handle(req)) {
        return std::make_unique<multipart>(m_dedupe_services, m_uploads);
    }
    if (init_multipart::can_handle(req)) {
        return std::make_unique<init_multipart>(m_directory, m_uploads);
    }
    if (complete_multipart::can_handle(req)) {
        return std::make_unique<complete_multipart>(m_directory, m_uploads,
                                                    m_limits);
    }
    if (list_objects_v2::can_handle(req)) {
        return std::make_unique<list_objects_v2>(m_directory);
    }
    if (list_objects::can_handle(req)) {
        return std::make_unique<list_objects>(m_directory);
    }
    if (list_buckets::can_handle(req)) {
        return std::make_unique<list_buckets>(m_directory);
    }
    if (head_object::can_handle(req)) {
        return std::make_unique<head_object>(m_directory);
    }
    if (head_bucket::can_handle(req)) {
        return std::make_unique<head_bucket>(m_directory);
    }
    if (create_bucket::can_handle(req)) {
        return std::make_unique<create_bucket>(m_directory);
    }
    if (copy_object::can_handle(req)) {
        return std::make_unique<copy_object>(m_directory, m_gdv);
    }
    if (list_multipart::can_handle(req)) {
        return std::make_unique<list_multipart>(m_uploads);
    }
    if (get_metrics::can_handle(req)) {
        return std::make_unique<get_metrics>(m_directory, m_gdv);
    }
    if (delete_object::can_handle(req)) {
        return std::make_unique<delete_object>(m_directory, m_gdv, m_limits);
    }
    if (delete_objects::can_handle(req)) {
        return std::make_unique<delete_objects>(m_directory, m_gdv, m_limits);
    }
    if (delete_bucket::can_handle(req)) {
        return std::make_unique<delete_bucket>(m_directory);
    }
    if (abort_multipart::can_handle(req)) {
        return std::make_unique<abort_multipart>(m_uploads);
    }

    throw command_exception(ep::http::status::bad_request, "CommandNotFound",
                            "no such command found");
}
limits& command_factory::get_limits() const { return m_limits; }
directory& command_factory::get_directory() const { return m_directory; }
} // namespace uh::cluster
