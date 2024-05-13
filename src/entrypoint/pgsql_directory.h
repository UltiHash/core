#ifndef UH_CLUSTER_DIRECTORY_PGSQL_DIRECTORY_H
#define UH_CLUSTER_DIRECTORY_PGSQL_DIRECTORY_H

#include "common/db/db.h"
#include "common/global_data/global_data_view.h"
#include "directory_interface.h"

namespace uh::cluster {

struct pgsql_directory : public directory_interface {

    pgsql_directory(db::database& db)
        : m_db(db) {}

    coro<void> put_object(const std::string& bucket,
                          const std::string& object_id,
                          const address& addr) override;

    coro<object> get_object(const std::string& bucket,
                            const std::string& object_id) override;

    coro<void> put_bucket(const std::string& bucket) override;

    coro<void> bucket_exists(const std::string& bucket) override;

    coro<void> delete_bucket(const std::string& bucket) override;

    coro<void> delete_object(const std::string& bucket,
                             const std::string& object_id) override;

    coro<std::vector<std::string>> list_buckets() override;

    coro<std::vector<object>>
    list_objects(const std::string& bucket,
                 const std::optional<std::string>& prefix,
                 const std::optional<std::string>& lower_bound) override;

private:
    db::database& m_db;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_DIRECTORY_INTERFACE_H
