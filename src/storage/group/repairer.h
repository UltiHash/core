#pragma once

#include "state.h"

#include <common/service_interfaces/hostport.h>

#include <memory>
#include <vector>

namespace uh::cluster::storage {

class repairer {
public:
    repairer(std::vector<std::shared_ptr<storage_state>> states,
             std::vector<std::shared_ptr<hostport>> hostports)
        : m_to_be_recovered{get_new_storage_indices(states)} {
        (void)hostports;
        // TODO: Spawn coroutines to restore data
        // After finishing recovery, we should set the storages' state to
        // ASSIGNED.
    }

    static std::vector<size_t> get_new_storage_indices(
        const std::vector<std::shared_ptr<storage_state>>& states) {
        std::vector<size_t> new_storage_indices;
        for (auto i = 0ul; i < states.size(); ++i) {
            if (*states[i] == storage_state::NEW) {
                new_storage_indices.push_back(i);
            }
        }
        return new_storage_indices;
    }

    bool is_changed(std::vector<std::shared_ptr<storage_state>> states) {
        auto new_indices = get_new_storage_indices(states);
        if (new_indices.size() != m_to_be_recovered.size()) {
            return true;
        }
        for (auto i = 0ul; i < new_indices.size(); ++i) {
            if (new_indices[i] != m_to_be_recovered[i]) {
                return true;
            }
        }
        return false;
    }

private:
    std::vector<size_t> m_to_be_recovered;
};

} // namespace uh::cluster::storage
