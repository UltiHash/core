#pragma once

#include "service_registry.h"

#include <common/etcd/utils.h>
#include <common/network/server.h>
#include <storage/group/state.h>

namespace uh::cluster::storage {

class storage_registry : public service_registry {

public:
    storage_registry(etcd_manager& etcd, std::size_t group_index,
                     std::size_t service_id,
                     const std::filesystem::path& working_dir);

    ~storage_registry() override;

    void register_service(const server_config& config) override;

    void update_service_state(const storage_state state);

private:
    std::string m_state_key;
    const std::filesystem::path m_working_dir;
    storage_state m_state = storage_state::NEW;

    bool read_state_from_disk(const std::filesystem::path& state_file);
    void write_state_to_disk(const std::filesystem::path& state_file,
                             storage_state state);
};

} // namespace uh::cluster::storage
