#pragma once

#include <functional>

#include "config.h"
#include "handler.h"

#include <common/etcd/registry/service_id.h>
#include <common/etcd/registry/service_registry.h>
#include <common/etcd/service.h>
#include <common/etcd/utils.h>
#include <common/license/license_watcher.h>
#include <common/network/server.h>
#include <common/utils/scope_guard.h>
#include <common/utils/strings.h>
#include <storage/global/config.h>
#include <storage/group/ec_maintainer.h>

namespace uh::cluster::storage {

class service {
public:
    service(const service_config& service_config, const storage_config& sc)
        : m_ioc(sc.server.threads),
          m_etcd{service_config.etcd_config},
          m_license_watcher(m_etcd),
          m_storage_id{sc.instance_id},
          m_group_id{sc.group_id},
          m_group_config{group_config::create([&]() -> std::string {
              LOG_DEBUG()
                  << "removing host_port group while initializing, on storage "
                  << m_storage_id;
              m_etcd.rm(ns::root.storage_groups[m_group_id]
                            .storage_hostports[m_storage_id]);
              LOG_DEBUG() << "waiting for group config to be ready, for group "
                          << m_group_id;
              return m_etcd.wait(
                  ns::root.storage_groups.group_configs[m_group_id],
                  time_settings::instance().get_group_state_wait_timeout());
          }())},
          m_storage(std::make_shared<local_storage>(m_storage_id, sc.data_store,
                                                    sc.working_directory)),

          m_server(sc.server, std::make_unique<handler>(*m_storage), m_ioc),
          m_service_registry(m_etcd,
                             ns::root.storage_groups[m_group_id]
                                 .storage_hostports[m_storage_id],
                             sc.server.port),
          m_ec_maintainer(
              (m_group_config.type == group_config::type_t::ERASURE_CODING)
                  ? std::make_optional<ec_maintainer<local_storage>>(
                        m_etcd, m_group_config, m_storage_id, service_config,
                        sc.global_data_view, m_storage)
                  : std::nullopt) {
        metric<storage_available_space_gauge, byte, int64_t>::
            register_gauge_callback(
                [this]() { return m_storage->get_available_space_func(); },
                [this]() {
                    auto label = m_license_watcher.get_license()
                                     ->to_key_value_iterable();
                    label.push_back(
                        {"service_id", std::to_string(m_storage_id)});
                    return label;
                });

        metric<storage_used_space_gauge, byte, int64_t>::
            register_gauge_callback(
                [this] { return m_storage->get_used_space_func(); },
                [this]() {
                    auto label = m_license_watcher.get_license()
                                     ->to_key_value_iterable();
                    label.push_back(
                        {"service_id", std::to_string(m_storage_id)});
                    return label;
                });

        auto g1 = scope_guard([]() {
            metric<storage_available_space_gauge, byte,
                   int64_t>::remove_gauge_callback();
        });
        auto g2 = scope_guard([]() {
            metric<storage_used_space_gauge, byte,
                   int64_t>::remove_gauge_callback();
        });
    }

    void run() { m_server.run(); }

    void stop() { m_server.stop(); }

    size_t id() const noexcept { return m_storage_id; }

    std::shared_ptr<local_storage> get_local_interface() { return m_storage; }

private:
    boost::asio::io_context m_ioc;
    etcd_manager m_etcd;
    license_watcher m_license_watcher;
    std::size_t m_storage_id;
    std::size_t m_group_id;
    group_config m_group_config;
    std::shared_ptr<local_storage> m_storage;
    server m_server;
    service_registry m_service_registry;
    std::optional<ec_maintainer<local_storage>> m_ec_maintainer;
};
} // namespace uh::cluster::storage
