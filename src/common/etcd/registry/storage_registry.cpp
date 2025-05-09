#include "storage_registry.h"

#include <common/etcd/namespace.h>
#include <common/utils/common.h>
#include <common/utils/strings.h>

#include <fstream>

using namespace boost::asio;

namespace uh::cluster::storage {

constexpr const char* STATE_FILE_NAME = "state";

storage_registry::storage_registry(etcd_manager& etcd, std::size_t group_id,
                                   std::size_t storage_id,
                                   const std::filesystem::path& working_dir)
    : m_etcd{etcd},
      m_prefix{ns::root.storage_groups[group_id].storage_states},
      m_storage_id{storage_id},
      m_file(working_dir / get_service_string(STORAGE_SERVICE) /
             STATE_FILE_NAME) {

    m_state = load();
    if (m_state == storage_state::DOWN) {
        m_state = storage_state::NEW;
        store(m_state);
    }

    publish();
}

storage_registry::~storage_registry() { m_etcd.rm(m_prefix[m_storage_id]); }

void storage_registry::set(const storage_state state) {
    LOG_DEBUG() << std::format("Set storage state to {}",
                               magic_enum::enum_name(state));
    m_state = state;
    store(m_state);
}

void storage_registry::publish() {
    m_etcd.put(m_prefix[m_storage_id], serialize(m_state));
}

void storage_registry::set_others_persistant(std::size_t id,
                                             storage_state value) {
    if (m_storage_id == id) {
        throw std::runtime_error("Cannot put storage state to itself");
    }
    m_etcd.put_persistant(m_prefix[id], serialize(value));
}

storage_state storage_registry::load() {

    if (!std::filesystem::exists(m_file)) {
        return storage_state::DOWN;
    }

    std::ifstream in(m_file, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("could not read file " + m_file.string());
    }

    uint8_t persisted_state;
    in.read(reinterpret_cast<char*>(&persisted_state), sizeof(uint8_t));

    const auto state_enum =
        magic_enum::enum_cast<storage_state>(persisted_state);
    if (!state_enum.has_value()) {
        return storage_state::DOWN;
    }
    LOG_DEBUG() << std::format("Load storage state: {}",
                               magic_enum::enum_name(state_enum.value()));
    return state_enum.value();
}

void storage_registry::store(storage_state state) {

    std::filesystem::create_directories(m_file.parent_path());

    std::ofstream out(m_file, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        throw std::runtime_error("could not open file " + m_file.string());
    }

    out.write(reinterpret_cast<const char*>(&state), sizeof(state));
}

} // namespace uh::cluster::storage
