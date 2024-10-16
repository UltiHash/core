#include "service.h"

#include "common/telemetry/metrics.h"
#include "common/utils/scope_guard.h"

namespace uh::cluster::ep {

namespace {

static const auto LIMITS_UPDATE_INTERVAL = std::chrono::seconds(5);

coro<void> update_limits(directory& dir, limits& l) {
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
    std::atomic<std::size_t> size = co_await dir.data_size();
    l.storage_size(size);

    metric<entrypoint_original_data_volume_gauge, byte,
           int64_t>::register_gauge_callback([&size]() {
        auto s = size.load();
        LOG_DEBUG() << "gauge_callback for original data: " << s;

        return s;
    });
    auto g = scope_guard([]() {
        metric<entrypoint_original_data_volume_gauge, byte,
               int64_t>::remove_gauge_callback();
    });

    while (true) {
        timer.expires_from_now(LIMITS_UPDATE_INTERVAL);
        co_await timer.async_wait(boost::asio::use_awaitable);

        size = co_await dir.data_size();
        l.storage_size(size);
    }
}

} // namespace

service::service(const service_config& sc, entrypoint_config config)
    : m_config(std::move(config)),
      m_ioc(boost::asio::io_context(m_config.server.threads)),

      m_etcd_client(make_etcd_client(sc.etcd_config)),
      m_service_id(get_service_id(m_etcd_client,
                                  get_service_string(ENTRYPOINT_SERVICE),
                                  sc.working_dir)),
      m_service_registry(ENTRYPOINT_SERVICE, m_service_id, m_etcd_client),

      m_attached_storage(sc, m_config.m_attached_storage),
      m_attached_dedupe(sc, m_config.m_attached_deduplicator),
      m_storage_maintainer(
          m_etcd_client,
          service_factory<storage_interface>(
              m_ioc, m_config.global_data_view.storage_service_connection_count,
              m_attached_storage.get_local_service_interface())),
      m_dedupe_maintainer(m_etcd_client,
                          service_factory<deduplicator_interface>(
                              m_ioc, m_config.dedupe_node_connection_count,
                              m_attached_dedupe.get_local_service_interface())),
      m_data_view(m_config.global_data_view, m_ioc, m_storage_maintainer),

      m_directory(m_ioc, m_config.database),
      m_uploads(m_ioc, m_config.database),
      m_users(m_ioc, m_config.database),
      m_limits(sc.license.max_data_store_size),
      m_server(m_config.server,
               std::make_unique<handler>(
                   command_factory(m_ioc, m_dedupe_load_balancer, m_directory,
                                   m_uploads, m_config, m_data_view, m_limits,
                                   m_users),
                   http::request_factory(m_users),
                   std::make_unique<policy::module>(m_directory)),
               m_ioc) {
    m_dedupe_maintainer.add_monitor(m_dedupe_load_balancer);

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
    m_registration =
        m_service_registry.register_service(m_server.get_server_config());
    m_server.run();
}

void service::stop() {
    LOG_INFO() << "stopping " << m_service_registry.get_service_name();
    m_server.stop();
}

service::~service() noexcept {
    m_dedupe_maintainer.remove_monitor(m_dedupe_load_balancer);
}

} // namespace uh::cluster::ep
