#include "storage_registry.h"

#include "common/etcd/namespace.h"
#include "common/utils/common.h"

#include <fstream>

using namespace boost::asio;

namespace uh::cluster {

constexpr const char* STATE_FILE_NAME = "state";

storage_registry::storage_registry(std::size_t service_index,
                                   std::size_t group_index, etcd_manager& etcd,
                                   const std::filesystem::path& working_dir)
    : service_registry(STORAGE_SERVICE, service_index, etcd),
      m_group_id(group_index),
      m_working_dir(working_dir) {}

storage_registry::~storage_registry() {
    service_registry::~service_registry();
    const std::string storage_group_path =
        get_storage_to_storage_group_map_path(m_id);
    const std::string storage_state_path =
        get_storage_groups_assigned_storages_path(m_group_id, m_id);
    m_etcd.rm(storage_group_path);
    m_etcd.rm(storage_state_path);
}

void storage_registry::register_service(const server_config& config) {
    service_registry::register_service(config);

    auto state_file =
        m_working_dir / get_service_string(STORAGE_SERVICE) / STATE_FILE_NAME;

    if (!read_state_from_disk(state_file)) {
        update_service_state(storage_state::NEW);
    }

    const std::string storage_group_path =
        get_storage_to_storage_group_map_path(m_id);
    m_etcd.put(storage_group_path, std::to_string(m_group_id));
}

void storage_registry::update_service_state(const storage_state state) {
    m_state = state;
    auto state_file =
        m_working_dir / get_service_string(STORAGE_SERVICE) / STATE_FILE_NAME;
    write_state_to_disk(state_file, state);

    const std::string storage_state_path =
        get_storage_groups_assigned_storages_path(m_group_id, m_id);
    const std::string value(std::to_string(magic_enum::enum_integer(state)));
    m_etcd.put(storage_state_path, value);
}

bool storage_registry::read_state_from_disk(
    const std::filesystem::path& state_file) {

    if (!std::filesystem::exists(state_file)) {
        return false;
    }

    std::ifstream in(state_file, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("could not read file " + state_file.string());
    }

    uint8_t persisted_state;
    in.read(reinterpret_cast<char*>(&persisted_state), sizeof(uint8_t));

    const auto state_enum =
        magic_enum::enum_cast<storage_registry::storage_state>(persisted_state);
    if (state_enum.has_value()) {
        m_state = state_enum.value();
        return true;
    }
    return false;
}

void storage_registry::write_state_to_disk(
    const std::filesystem::path& state_file,
    storage_registry::storage_state state) {

    std::filesystem::create_directories(state_file.parent_path());

    std::ofstream out(state_file, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        throw std::runtime_error("could not open file " + state_file.string());
    }

    out.write(reinterpret_cast<const char*>(&state), sizeof(uint8_t));
}

} // namespace uh::cluster
