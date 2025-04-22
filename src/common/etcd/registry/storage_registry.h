#pragma once

#include "common/etcd/utils.h"
#include "common/network/server.h"
#include "service_registry.h"

namespace uh::cluster::storage {

enum class storage_state : std::uint8_t { DOWN, NEW, ASSIGNED };

class storage_registry : public service_registry {

public:
    storage_registry(std::size_t service_index, std::size_t group_index,
                     etcd_manager& etcd,
                     const std::filesystem::path& working_dir);

    ~storage_registry() override;

    void register_service(const server_config& config) override;

    void update_service_state(const storage_state state);

private:
    const size_t m_group_id;
    const std::filesystem::path m_working_dir;
    storage_state m_state = storage_state::NEW;

    bool read_state_from_disk(const std::filesystem::path& state_file);
    void write_state_to_disk(const std::filesystem::path& state_file,
                             storage_state state);
};

} // namespace uh::cluster::storage
