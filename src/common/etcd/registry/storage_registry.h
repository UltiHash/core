#pragma once

#include "common/etcd/utils.h"
#include "common/network/server.h"
#include "service_registry.h"
#include <string>

namespace uh::cluster {

class storage_registry : public service_registry {

public:
    enum class storage_state : std::uint8_t {
        NEW = 0,
        ASSIGNED = 1,
    };

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
                             storage_registry::storage_state state);
};

} // namespace uh::cluster
