#include "service.h"
#include "handler.h"

#include <common/telemetry/metrics.h>
#include <common/utils/scope_guard.h>
#include <deduplicator/interfaces/dedupe_array.h>
#include <deduplicator/interfaces/noop_deduplicator.h>
#include <storage/interfaces/global_data_view.h>
#include <storage/interfaces/null_storage.h>

namespace uh::cluster::ep {

namespace {

static const auto LIMITS_UPDATE_INTERVAL = std::chrono::seconds(5);

coro<void> update_limits(uh::cluster::directory& directory, limits& l) {
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
    std::atomic<std::size_t> size = co_await directory.data_size();
    l.set_storage_size(size);

    metric<entrypoint_original_data_volume_gauge, byte,
           int64_t>::register_gauge_callback([&size]() { return size.load(); });
    auto g = scope_guard([]() {
        metric<entrypoint_original_data_volume_gauge, byte,
               int64_t>::remove_gauge_callback();
    });

    while (true) {
        timer.expires_after(LIMITS_UPDATE_INTERVAL);
        co_await timer.async_wait(boost::asio::use_awaitable);

        size = co_await directory.data_size();
        l.set_storage_size(size);
    }
}

std::unique_ptr<deduplicator_interface>
make_deduplicator(const entrypoint_config& config, sn::interface& storage,
                  boost::asio::io_context& ioc, etcd_manager& etcd) {

    switch (config.deduplicator) {
    case dd_type::local:
        LOG_INFO() << "using deduplicator: local";
        return std::make_unique<local_deduplicator>(
            ioc, config.local_deduplicator, storage);

    case dd_type::null:
        LOG_INFO() << "using deduplicator: noop";
        return std::make_unique<noop_deduplicator>(storage);

    case dd_type::cluster:
        LOG_INFO() << "using deduplicator: cluster";
        return std::make_unique<dedupe_array>(
            ioc, etcd, config.dedupe_node_connection_count);
    }

    throw std::runtime_error("unknown deduplicator type");
}

std::unique_ptr<sn::interface> make_storage(const entrypoint_config& config,
                                            boost::asio::io_context& ioc,
                                            etcd_manager& etcd) {
    switch (config.storage) {
    case sn_type::local:
        LOG_INFO() << "using storage: local";
        return std::make_unique<local_storage>(
            0, config.local_storage.data_store,
            config.local_storage.m_data_store_roots);

    case sn_type::cluster:
        LOG_INFO() << "using storage: cluster";
        return std::make_unique<global_data_view>(config.cluster_storage, ioc,
                                                  etcd);
    case sn_type::null:
        LOG_INFO() << "using storage: null";
        return std::make_unique<null_storage>();
    }

    throw std::runtime_error("unknown storage type");
}

} // namespace

service::service(const service_config& sc, entrypoint_config config)
    : m_config(std::move(config)),
      m_ioc(boost::asio::io_context(m_config.server.threads)),
      m_etcd{sc.etcd_config},
      m_service_id(get_service_id(
          m_etcd, get_service_string(ENTRYPOINT_SERVICE), sc.working_dir)),
      m_service_registry(ENTRYPOINT_SERVICE, m_service_id, m_etcd),
      m_storage(make_storage(m_config, m_ioc, m_etcd)),
      m_dedupe(make_deduplicator(m_config, *m_storage, m_ioc, m_etcd)),

      m_directory(m_ioc, m_config.database),
      m_uploads(m_ioc, m_config.database),
      m_users(m_ioc, m_config.database),
      m_license_watcher(m_etcd),
      m_limits(m_license_watcher),
      m_server(m_config.server,
               std::make_unique<handler>(
                   command_factory(m_ioc, *m_dedupe, m_directory, m_uploads,
                                   m_config, *m_storage, m_limits, m_users),
                   http::request_factory(m_users),
                   std::make_unique<policy::module>(m_directory)),
               m_ioc),
      m_gc(m_ioc, m_directory, *m_storage) {
    co_spawn(
        m_ioc, update_limits(m_directory, m_limits), [](std::exception_ptr e) {
            try {
                std::rethrow_exception(e);
            } catch (const std::exception& e) {
                LOG_ERROR() << "metrics monitor stopped working: " << e.what();
            }
        });
}

void service::run() {
    m_service_registry.register_service(m_server.get_server_config());
    m_server.run();
}

void service::stop() {
    LOG_INFO() << "stopping " << m_service_registry.get_service_name();

    m_server.stop();
}

} // namespace uh::cluster::ep
