#ifndef UH_CLUSTER_ENTRYPOINT_DIRECTORY_H
#define UH_CLUSTER_ENTRYPOINT_DIRECTORY_H

#include "common/db/db.h"
#include "common/network/messenger_core.h"

namespace uh::cluster {

enum class bucket_delete_policy { only_empty, all };

struct directory {

    directory(db::database& db)
        : m_db(db) {}

    coro<void> put_object(const std::string& bucket,
                          const std::string& object_id, const address& addr);

    coro<object> get_object(const std::string& bucket,
                            const std::string& object_id);

    coro<object> head_object(const std::string& bucket,
                             const std::string& object_id);

    coro<void> put_bucket(const std::string& bucket);

    coro<void> bucket_exists(const std::string& bucket);

    coro<void> delete_bucket(const std::string& bucket);

    coro<void> delete_object(const std::string& bucket,
                             const std::string& object_id);

    coro<std::vector<std::string>> list_buckets();

    coro<std::vector<object>>
    list_objects(const std::string& bucket,
                 const std::optional<std::string>& prefix,
                 const std::optional<std::string>& lower_bound);

    /**
     * Return amount of data stored in all buckets.
     */
    coro<std::size_t> data_size();

private:
    db::database& m_db;

    bucket_delete_policy m_bucket_delete_policy = bucket_delete_policy::all;
};

} // namespace uh::cluster

#endif
