#pragma once

#include <functional>
#include <utility>

#include "config.h"
#include "handler.h"
#include <common/etcd/registry/service_id.h>
#include <common/etcd/registry/storage_registry.h>
#include <common/etcd/service.h>
#include <common/license/license_watcher.h>
#include <common/network/server.h>
#include <common/utils/scope_guard.h>

namespace uh::cluster::storage {

class service {
public:
    service(const service_config& service, const storage_config& sc)
        : m_ioc(sc.server.threads),
          m_etcd{service.etcd_config},
          m_license_watcher(m_etcd),
          m_service_id(register_storage_service_id(m_etcd, sc.service_id)),
          m_storage(std::make_shared<local_storage>(m_service_id, sc.data_store,
                                                    sc.m_data_store_roots)),
          m_storage_registry(m_service_id, 0, m_etcd, service.working_dir),
          m_server(sc.server,
                   std::make_unique<handler>(*m_storage, m_storage_registry),
                   m_ioc) {}

    void run() {
        metric<storage_available_space_gauge, byte, int64_t>::
            register_gauge_callback(
                [this]() { return m_storage->get_available_space_func(); },
                [this]() {
                    auto label = m_license_watcher.get_license()
                                     ->to_key_value_iterable();
                    label.push_back(
                        {"service_id", std::to_string(m_service_id)});
                    return label;
                });

        metric<storage_used_space_gauge, byte, int64_t>::
            register_gauge_callback(
                [this] { return m_storage->get_used_space_func(); },
                [this]() {
                    auto label = m_license_watcher.get_license()
                                     ->to_key_value_iterable();
                    label.push_back(
                        {"service_id", std::to_string(m_service_id)});
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

        m_storage_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() { m_server.stop(); }

    size_t id() const noexcept { return m_service_id; }

    std::shared_ptr<local_storage> get_local_interface() { return m_storage; }

private:
    boost::asio::io_context m_ioc;
    etcd_manager m_etcd;
    license_watcher m_license_watcher;
    std::size_t m_service_id;
    std::shared_ptr<local_storage> m_storage;
    storage_registry m_storage_registry;
    server m_server;
};
} // namespace uh::cluster::storage
