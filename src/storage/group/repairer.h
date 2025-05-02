#pragma once

#include "state.h"

#include <common/service_interfaces/storage_interface.h>

#include <memory>
#include <vector>

namespace uh::cluster::storage {

class repairer {
public:
    repairer(std::vector<std::shared_ptr<storage_state>> states,
             std::vector<std::shared_ptr<storage_interface>> storages) {
        (void)storages;
        // TODO: Spawn coroutines to restore data
        // After finishing recovery, we should set the storages' state to
        // ASSIGNED.
    }
};

} // namespace uh::cluster::storage
