#pragma once

#include "limits.h"

#include <common/etcd/utils.h>
#include <common/license/payg_watcher.h>
#include <common/license/test.h>

namespace uh::cluster::ep {

class license_manager {
public:
    license_manager(etcd_manager& etcd, limits& limits,
                    const uh::cluster::license& test_license) {
        if (test_license.max_data_store_size != 0) {
            limits.set_storage_cap(test_license.max_data_store_size);
        } else {
            m_watcher.emplace(etcd, [&](const lic::payg& license) {
                limits.set_storage_cap(license.storage_cap);
            });
        }
    }

private:
    std::optional<lic::payg_watcher> m_watcher;
};

} // namespace uh::cluster::ep
