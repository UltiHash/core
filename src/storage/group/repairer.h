#pragma once

#include <common/execution/executor.h>
#include <common/service_interfaces/storage_interface.h>
#include <storage/global/config.h>
#include <storage/group/config.h>
#include <storage/group/externals.h>

namespace uh::cluster::storage {

class repairer {
public:
    repairer(executor& executor, etcd_manager& etcd, const group_config& config,
             std::size_t storage_id,
             const global_data_view_config& global_config)
        : m_executor{executor},
          m_etcd{etcd},
          m_group_config{config},
          m_global_config{global_config},
          m_subscriber(m_etcd, m_group_config.id, m_group_config.storages,
                       service_factory<storage_interface>(
                           m_executor.get_executor(),
                           m_global_config.storage_service_connection_count)) {
        (void)m_subscriber;
        // TODO: Spawn coroutines to restore data
        //
        // storage_assignment_triggers_manager::put(m_etcd, m_group_config.id,
        //                                          true);
    }

private:
    executor& m_executor;
    etcd_manager& m_etcd;
    group_config m_group_config;
    global_data_view_config m_global_config;

    externals_subscriber m_subscriber;
};

} // namespace uh::cluster::storage
