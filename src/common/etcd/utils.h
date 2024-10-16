#ifndef CORE_COMMON_ETCD_UTILS_H
#define CORE_COMMON_ETCD_UTILS_H

#include <etcd/SyncClient.hpp>
#include <optional>
#include <string>

namespace uh::cluster {

struct etcd_config {
    // URL of the etcd service
    std::string url = "http://127.0.0.1:2379";

    std::optional<std::string> username;
    std::optional<std::string> password;
};

/**
 * Create etcd client
 */
::etcd::SyncClient make_etcd_client(const etcd_config& cfg);

} // namespace uh::cluster

#endif
