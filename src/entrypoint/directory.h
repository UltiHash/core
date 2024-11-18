#ifndef UH_CLUSTER_ENTRYPOINT_DIRECTORY_H
#define UH_CLUSTER_ENTRYPOINT_DIRECTORY_H

#include "common/db/db.h"
#include "common/global_data/global_data_view.h"
#include "common/types/common_types.h"
#include "common/utils/pool.h"
#include "common/utils/scope_guard.h"

#include <functional>

namespace uh::cluster {

enum class bucket_delete_policy { only_empty, all };

class directory {
public:
    directory(boost::asio::io_context& ioc, const db::config& cfg)
        : m_db(ioc, connection_factory(ioc, cfg, cfg.directory),
               cfg.directory.count) {}

    class instance {
    public:
        using object_lock = guard<std::function<void()>>;

        instance(instance&& other) = default;
        instance(const instance&) = delete;

        coro<void> put_object(const std::string& bucket, const object& obj);

        coro<object> get_object(const std::string& bucket,
                                const std::string& object_id);

        coro<object> head_object(const std::string& bucket,
                                 const std::string& object_id);

        object_lock lock_object(const std::string& bucket,
                                const std::string& object_id);

        object_lock lock_object_shared(const std::string& bucket,
                                       const std::string& object_id);

        coro<void> put_bucket(const std::string& bucket);

        coro<void> bucket_exists(const std::string& bucket);

        coro<void> delete_bucket(const std::string& bucket);

        coro<void> delete_object(const std::string& bucket,
                                 const std::string& object_id);

        coro<std::vector<std::string>> list_buckets();

        coro<std::optional<std::string>>
        get_bucket_policy(const std::string& bucket);

        coro<void> set_bucket_policy(const std::string& bucket,
                                     std::optional<std::string> policy);

        coro<std::vector<object>>
        list_objects(const std::string& bucket,
                     const std::optional<std::string>& prefix,
                     const std::optional<std::string>& lower_bound);

        /**
         * Return amount of data stored in all buckets.
         */
        coro<std::size_t> data_size();

    private:
        friend class directory;
        instance(pool<db::connection>::handle&& handle);
        pool<db::connection>::handle m_handle;
    };

    coro<instance> get();

private:
    pool<db::connection> m_db;

    static void validate_bucket_name(const std::string& bucket_name);
};

/**
 * Convenience function to safely put an object. Returns the amount of reclaimed
 * memory.
 *
 * Before writing the new object data it will retrieve data already stored. This
 * data will be freed be unlinking it.
 *
 * If there is any error during execution, the function will unlink the object's
 * address data and set it to empty.
 *
 * @param ctx the request context
 * @param dir a directory instance
 * @param gdv reference to the global data view
 * @param bucket name of the bucket to work in
 * @param obj object specification to write, including object name
 *
 * @return number of bytes reclaimed
 */
coro<std::size_t> safe_put_object(context& ctx, directory::instance& dir,
                                  global_data_view& gdv,
                                  const std::string& bucket, object& obj);

} // namespace uh::cluster

#endif
