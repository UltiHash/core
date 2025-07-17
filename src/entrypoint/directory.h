#pragma once

#include <common/db/db.h>
#include <common/utils/scope_guard.h>
#include <storage/global/data_view.h>

#include "object.h"

#include <functional>

namespace uh::cluster {

using ep::object;

enum class bucket_versioning { disabled, enabled, suspended };

std::string to_string(bucket_versioning versioning);
bucket_versioning to_versioning(std::string s);

class directory {
public:
    directory(boost::asio::io_context& ioc, const db::config& cfg)
        : m_db{ioc, connection_factory(ioc, cfg, cfg.directory),
               cfg.directory.count} {}

    struct unref {
        promise<void> p;

        void operator()();
    };

    using object_lock = value_guard<object, unref>;

    coro<std::optional<std::string>> put_object(const std::string& bucket,
                                                const object& obj);

    coro<object_lock>
    get_object(const std::string& bucket, const std::string& object_id,
               std::optional<std::string> version = std::nullopt);

    coro<object> head_object(const std::string& bucket,
                             const std::string& object_id,
                             std::optional<std::string> version = std::nullopt);

    coro<void> put_bucket(const std::string& bucket);

    coro<void> bucket_exists(const std::string& bucket);

    coro<void> delete_bucket(const std::string& bucket);

    struct delete_result {
        bool is_delete_marker;
        std::optional<std::string> version;
    };
    coro<delete_result>
    delete_object(const std::string& bucket, const std::string& object_id,
                  std::optional<std::string> version = std::nullopt);

    coro<std::vector<std::string>> list_buckets();

    coro<std::optional<std::string>>
    get_bucket_policy(const std::string& bucket);

    coro<void> set_bucket_policy(const std::string& bucket,
                                 std::optional<std::string> policy);

    coro<std::optional<std::string>> get_bucket_cors(const std::string& bucket);

    coro<void> set_bucket_cors(const std::string& bucket,
                               std::optional<std::string> cors);

    coro<bucket_versioning> get_bucket_versioning(const std::string& bucket);
    coro<void> set_bucket_versioning(const std::string& bucket,
                                     bucket_versioning versioning);

    coro<std::vector<object>>
    list_objects(const std::string& bucket,
                 const std::optional<std::string>& prefix,
                 const std::optional<std::string>& lower_bound);

    coro<std::vector<object>> list_object_versions(
        const std::string& bucket, const std::optional<std::string>& prefix,
        const std::optional<std::string>& key_marker,
        const std::optional<std::string>& version_marker, std::size_t limit);

    struct to_delete {
        std::size_t id;
        address addr;
    };
    coro<std::optional<to_delete>> next_deleted();

    coro<void> clear_buckets();

    coro<void> remove_object(std::size_t id);

    /**
     * Return amount of data stored in all buckets.
     */
    coro<std::size_t> data_size();

private:
    pool<db::connection> m_db;

    static void validate_bucket_name(const std::string& bucket_name);
};

/**
 * Convenience function to safely put an object. Returns the version of the
 * object if versions are enabled for the bucket.
 *
 * Before writing the new object data it will retrieve data already stored. This
 * data will be freed be unlinking it.
 *
 * If there is any error during execution, the function will unlink the object's
 * address data and set it to empty.
 *
 * @param dir a directory
 * @param gdv reference to the global data view
 * @param bucket name of the bucket to work in
 * @param obj object specification to write, including object name
 */
coro<std::optional<std::string>>
safe_put_object(directory& dir, storage::global::global_data_view& gdv,
                const std::string& bucket, const object& obj);

} // namespace uh::cluster
