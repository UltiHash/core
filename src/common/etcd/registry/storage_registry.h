#pragma once

#include <common/etcd/utils.h>
#include <common/telemetry/log.h>
#include <common/utils/strings.h>
#include <fstream>
#include <magic_enum/magic_enum.hpp>
#include <storage/group/state.h>

namespace uh::cluster::storage {

class storage_registry {

public:
    storage_registry(etcd_manager& etcd, std::size_t group_id,
                     std::size_t storage_id,
                     const std::filesystem::path& working_dir)
        : m_etcd{etcd},
          m_prefix{ns::root.storage_groups[group_id].storage_states},
          m_storage_id{storage_id},
          m_file(working_dir / get_service_string(STORAGE_SERVICE) / "state") {

        m_state = load();
        if (m_state == storage_state::DOWN) {
            m_state = storage_state::NEW;
            store(m_state);
        }
        publish();
    }

    ~storage_registry() {
        LOG_DEBUG() << std::format("Destroy storage registry for {}",
                                   m_storage_id);
        m_etcd.rm(m_prefix[m_storage_id]);
    }

    void set(storage_state state) {
        // NOTE: now we need to skip repeated set call
        if (m_state != state) {
            LOG_DEBUG() << std::format("Set storage {} state to {}",
                                       m_storage_id,
                                       magic_enum::enum_name(state));
            m_state = state;
            store(m_state);
        }
        publish();
    }

private:
    void publish() { m_etcd.put(m_prefix[m_storage_id], serialize(m_state)); }

    etcd_manager& m_etcd;

    ns::subscriptable_key_t m_prefix;
    std::size_t m_storage_id;
    const std::filesystem::path m_file;
    storage_state m_state;

    storage_state load() {

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

    void store(storage_state state) {

        std::filesystem::create_directories(m_file.parent_path());

        std::ofstream out(m_file, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            throw std::runtime_error("could not open file " + m_file.string());
        }

        out.write(reinterpret_cast<const char*>(&state), sizeof(state));
    }
};

} // namespace uh::cluster::storage
