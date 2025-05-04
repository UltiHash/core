#pragma once

#include "service_registry.h"

#include <common/etcd/utils.h>
#include <common/network/server.h>
#include <storage/group/state.h>

namespace uh::cluster::storage {

class storage_registry {

public:
    storage_registry(etcd_manager& etcd, std::size_t group_index,
                     std::size_t service_id,
                     const std::filesystem::path& working_dir);
    ~storage_registry();

    void update_service_state(const storage_state state);

private:
    etcd_manager& m_etcd;

    std::string m_state_key;
    const std::filesystem::path m_file;

    storage_state load();
    void store(storage_state state);
};

} // namespace uh::cluster::storage
