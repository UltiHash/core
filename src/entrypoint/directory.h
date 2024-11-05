#ifndef UH_CLUSTER_ENTRYPOINT_DIRECTORY_H
#define UH_CLUSTER_ENTRYPOINT_DIRECTORY_H

#include "common/db/db.h"
#include "common/network/messenger_core.h"
#include "common/types/common_types.h"

namespace uh::cluster {

enum class bucket_delete_policy { only_empty, all };

class directory {
public:
    directory(boost::asio::io_context& ioc, const db::config& cfg)
        : m_db(ioc, connection_factory(ioc, cfg, cfg.directory),
               cfg.directory.count) {}

    class instance {
    public:
        instance(instance&& other) = default;
        instance(const instance&) = delete;

        db::connection* operator->();

        coro<void> put_object(const std::string& bucket, const object& obj);

        coro<object> get_object(const std::string& bucket,
                                const std::string& object_id);

        coro<object> head_object(const std::string& bucket,
                                 const std::string& object_id);

        coro<db::connection::transaction>
        lock_object(const std::string& bucket, const std::string& object_id);

        coro<db::connection::transaction>
        lock_object_shared(const std::string& bucket,
                           const std::string& object_id);

        coro<void> put_bucket(const std::string& bucket);

        coro<void> bucket_exists(const std::string& bucket);

        coro<void> delete_bucket(const std::string& bucket);

        coro<void> delete_object(const std::string& bucket,
                                 const std::string& object_id);

        coro<void> copy_object(const std::string& bucket_src,
                               const std::string& key_src,
                               const std::string& bucket_dst,
                               const std::string& key_dst);

        coro<void> copy_object_ifmatch(const std::string& bucket_src,
                                       const std::string& key_src,
                                       const std::string& bucket_dst,
                                       const std::string& key_dst,
                                       const std::string& etag);

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

} // namespace uh::cluster

#endif
