#ifndef CORE_ENTRYPOINT_SERVICE_H
#define CORE_ENTRYPOINT_SERVICE_H

#include <functional>

#include "common/db/db.h"
#include "common/etcd/registry/service_id.h"
#include "common/etcd/registry/service_registry.h"
#include "config.h"
#include "deduplicator/deduplicator.h"
#include "entrypoint/directory.h"
#include "entrypoint/garbage_collector.h"
#include "entrypoint/http/request_factory.h"
#include "entrypoint/limits.h"
#include "handler.h"

namespace uh::cluster::ep {

class service {
public:
    explicit service(const service_config& sc, entrypoint_config config);

    void run();

    void stop();

    ~service() noexcept;

private:
    entrypoint_config m_config;

    boost::asio::io_context m_ioc;
    std::unique_ptr<etcd::SyncClient> m_etcd_client;
    std::size_t m_service_id;
    service_registry m_service_registry;
    std::unique_ptr<service_registry::registration> m_registration;

    attached_service<storage> m_attached_storage;
    attached_service<deduplicator> m_attached_dedupe;

    service_maintainer<storage_interface> m_storage_maintainer;
    service_maintainer<deduplicator_interface> m_dedupe_maintainer;
    roundrobin_load_balancer<deduplicator_interface> m_dedupe_load_balancer;

    global_data_view m_data_view;
    directory m_directory;

    multipart_state m_uploads;
    user::db m_users;
    limits m_limits;
    server m_server;
    garbage_collector m_gc;
};

} // namespace uh::cluster::ep

#endif // CORE_ENTRY_NODE_H
