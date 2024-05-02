
#ifndef UH_CLUSTER_DIRECTORY_INTERFACE_H
#define UH_CLUSTER_DIRECTORY_INTERFACE_H

#include "common/network/messenger_core.h"

namespace uh::cluster {

struct directory_interface {

    struct read_handle {
        virtual bool has_next() = 0;
        virtual coro<std::string> next() = 0;
        virtual ~read_handle() = default;
    };

    static constexpr role service_role = DIRECTORY_SERVICE;

    virtual coro<void> put_object(const std::string& bucket,
                                  const std::string& object_id,
                                  const address& addr) = 0;
    virtual coro<std::unique_ptr<read_handle>>
    get_object(const std::string& bucket, const std::string& object_id) = 0;
    virtual coro<void> put_bucket(const std::string& bucket) = 0;
    virtual coro<void> bucket_exists(const std::string& bucket) = 0;
    virtual coro<void> delete_bucket(const std::string& bucket) = 0;
    virtual coro<void> delete_object(const std::string& bucket,
                                     const std::string& object_id) = 0;
    virtual coro<std::vector<std::string>> list_buckets() = 0;
    virtual coro<std::vector<object>>
    list_objects(const std::string& bucket,
                 const std::optional<std::string>& prefix,
                 const std::optional<std::string>& lower_bound) = 0;

    virtual ~directory_interface() = default;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_DIRECTORY_INTERFACE_H
