#pragma once

#include <common/etcd/service_discovery/service_load_balancer.h>
#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/etcd/service_discovery/storage_index.h>
#include <common/etcd/utils.h>
#include <common/service_interfaces/service_factory.h>
#include <common/service_interfaces/storage_interface.h>
#include <common/telemetry/log.h>
#include <common/types/common_types.h>
#include <common/utils/strings.h>
#include <storage/global/data_view.h>
#include <storage/group/config.h>

namespace uh::cluster::storage {

// TODO:
// Implement ec_group_manager using externals_subscriber
class group_manager {
public:
    group_manager(boost::asio::io_context& ioc, etcd_manager& etcd,
                  std::size_t group_id,
                  std::size_t storage_service_connection_count)
        : m_ioc{ioc},
          m_etcd{etcd},
          m_group_id{group_id},
          m_group_config{deserialize<storage::group_config>(
              m_etcd.wait(ns::root.storage_groups.group_configs[m_group_id],
                          SERVICE_GET_TIMEOUT))},
          m_load_balancer{},
          m_storage_index{m_group_config.storages},

          m_storage_maintainer(m_etcd,
                               ns::root.storage_groups[0].storage_hostports,
                               service_factory<storage_interface>(
                                   m_ioc, storage_service_connection_count),
                               {m_load_balancer, m_storage_index}),

          m_data_view(m_ioc, m_load_balancer, m_storage_index) {}

    auto& get_data_view() { return m_data_view; }

private:
    boost::asio::io_context& m_ioc;
    etcd_manager& m_etcd;
    std::size_t m_group_id;
    storage::group_config m_group_config;
    service_load_balancer<storage_interface> m_load_balancer;
    storage_index m_storage_index;

    // TODO: do not maintain storage services here. We can use
    // externals_subscriber instance.
    service_maintainer<storage_interface> m_storage_maintainer;

    storage::global::global_data_view m_data_view;
};

} // namespace uh::cluster::storage
