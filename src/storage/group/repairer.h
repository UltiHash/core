#pragma once

#include <common/service_interfaces/storage_interface.h>
#include <storage/global/config.h>
#include <storage/group/config.h>
#include <storage/group/externals.h>

namespace uh::cluster::storage {

class repairer {
public:
    repairer(boost::asio::io_context& ioc, etcd_manager& etcd,
             const group_config& config, std::size_t storage_id,
             const global_data_view_config& global_config)
        : m_ioc{ioc},
          m_etcd{etcd},
          m_group_config{config},
          m_global_config{global_config},
          m_subscriber(
              m_etcd, m_group_config.id, m_group_config.storages,
              service_factory<storage_interface>(
                  m_ioc, m_global_config.storage_service_connection_count)) {
        (void)m_subscriber;
        // TODO: Spawn coroutines to restore data
        // After finishing recovery, we should set the storages' state to
        // ASSIGNED.
    }

private:
    boost::asio::io_context& m_ioc;
    etcd_manager& m_etcd;
    group_config m_group_config;
    global_data_view_config m_global_config;

    externals_subscriber m_subscriber;
};

} // namespace uh::cluster::storage
