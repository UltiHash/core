#pragma once

#include "config.h"
#include "multipart_state.h"

#include <common/db/db.h>
#include <common/etcd/registry/service_id.h>
#include <common/etcd/registry/service_registry.h>
#include <common/global_data/default_global_data_view.h>
#include <common/license/license_watcher.h>
#include <deduplicator/service.h>
#include <entrypoint/directory.h>
#include <entrypoint/garbage_collector.h>
#include <entrypoint/http/request_factory.h>
#include <entrypoint/limits.h>

namespace uh::cluster::ep {

class service {
public:
    service(const service_config& sc, entrypoint_config config);

    void run();

    void stop();

private:
    entrypoint_config m_config;
    boost::asio::io_context m_ioc;
    etcd_manager m_etcd;
    std::size_t m_service_id;
    service_registry m_service_registry;

    attached_service<storage::service> m_attached_storage;
    service_maintainer<storage_interface> m_storage_maintainer;

    default_global_data_view m_data_view;
    std::unique_ptr<deduplicator_interface> m_dedupe;
    directory m_directory;

    multipart_state m_uploads;
    user::db m_users;
    license_watcher m_license_watcher;
    limits m_limits;
    server m_server;
    garbage_collector m_gc;
};

} // namespace uh::cluster::ep
