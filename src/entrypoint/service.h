#pragma once

#include "config.h"
#include "multipart_state.h"

#include <common/db/db.h>
#include <common/etcd/registry/service_id.h>
#include <common/etcd/registry/service_registry.h>
#include <common/global_data/default_global_data_view.h>
#include <deduplicator/service.h>
#include <entrypoint/directory.h>
#include <entrypoint/garbage_collector.h>
#include <entrypoint/http/request_factory.h>
#include <entrypoint/limits.h>

namespace uh::cluster::ep {

class payg_manager {
public:
    payg_manager(etcd_manager& etcd, std::atomic<std::size_t>& storage_cap)
        : m_etcd{etcd} {

        storage_cap.store(
            m_etcd.get<std::size_t>(get_etcd_payg_license_key("storage_cap")));
        m_wg = m_etcd.watch(get_etcd_payg_license_key("storage_cap"),
                            [&](etcd::Response) {
                                storage_cap.store(m_etcd.get<std::size_t>(
                                    get_etcd_payg_license_key("storage_cap")));
                            });
    }

private:
    etcd_manager& m_etcd;
    etcd_manager::watch_guard m_wg;
};

class license_manager {
public:
    license_manager(etcd_manager& etcd,
                    const uh::cluster::license& test_license) {
        // FIXIT: We need a way to check test license's invalidity.
        if (test_license.max_data_store_size != 0) {
            storage_cap.store(test_license.max_data_store_size);
        } else {
            m_payg_manager = std::make_unique<payg_manager>(etcd, storage_cap);
        }
    }

    std::atomic<std::size_t>& get_storage_cap() { return storage_cap; }

private:
    std::unique_ptr<payg_manager> m_payg_manager;
    std::atomic<std::size_t> storage_cap;
};

class service {
public:
    service(const service_config& sc, entrypoint_config config);

    void run();

    void stop();

    ~service() noexcept;

private:
    entrypoint_config m_config;
    boost::asio::io_context m_ioc;
    etcd_manager m_etcd;
    std::size_t m_service_id;
    service_registry m_service_registry;

    attached_service<storage::service> m_attached_storage;
    attached_service<deduplicator::service> m_attached_dedupe;

    service_maintainer<storage_interface> m_storage_maintainer;
    service_maintainer<deduplicator_interface> m_dedupe_maintainer;
    roundrobin_load_balancer<deduplicator_interface> m_dedupe_load_balancer;

    default_global_data_view m_data_view;
    directory m_directory;

    multipart_state m_uploads;
    user::db m_users;
    license_manager m_license_manager;
    limits m_limits;
    server m_server;
    garbage_collector m_gc;
};

} // namespace uh::cluster::ep
