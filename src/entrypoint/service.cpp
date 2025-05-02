#include "service.h"
#include "handler.h"

#include <common/telemetry/metrics.h>
#include <common/utils/scope_guard.h>
#include <deduplicator/interfaces/dedupe_array.h>
#include <deduplicator/interfaces/noop_deduplicator.h>
#include <format>
#include <magic_enum/magic_enum.hpp>

namespace uh::cluster::ep {

namespace {

static const auto LIMITS_UPDATE_INTERVAL = std::chrono::seconds(5);

coro<void> update_limits(uh::cluster::directory& directory, limits& l) {
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
    std::atomic<std::size_t> size = co_await directory.data_size();
    l.set_storage_size(size);

    while (true) {
        timer.expires_after(LIMITS_UPDATE_INTERVAL);
        co_await timer.async_wait(boost::asio::use_awaitable);

        size = co_await directory.data_size();
        l.set_storage_size(size);
    }
}

std::unique_ptr<deduplicator_interface>
make_deduplicator(const entrypoint_config& config,
                  storage::global::global_data_view& storage,
                  storage::global::cache& cache, boost::asio::io_context& ioc,
                  etcd_manager& etcd) {

    if (config.m_attached_deduplicator) {
        LOG_INFO() << "using attached deduplicator";
        return std::make_unique<local_deduplicator>(
            *config.m_attached_deduplicator, storage, cache);
    }

    if (config.noop_deduplicator) {
        LOG_INFO() << "using noop deduplicator";
        return std::make_unique<noop_deduplicator>(storage);
    }

    LOG_INFO() << "using remote deduplicator array";
    return std::make_unique<dedupe_array>(ioc, etcd,
                                          config.dedupe_node_connection_count);
}

} // namespace

service::service(const service_config& sc, entrypoint_config config)
    : m_config(std::move(config)),
      m_ioc(boost::asio::io_context(m_config.server.threads)),
      m_etcd{sc.etcd_config},
      m_service_id(get_service_id(
          m_etcd, get_service_string(ENTRYPOINT_SERVICE), sc.working_dir)),
      m_service_registry(m_etcd, ns::root.entrypoint.hostports[m_service_id]),

      m_gdv{m_ioc, m_etcd, config.global_data_view},
      m_cache(m_ioc, m_gdv, config.global_data_view.read_cache_capacity_l2),

      m_dedupe(make_deduplicator(m_config, m_gdv, m_cache, m_ioc, m_etcd)),

      m_directory(m_ioc, m_config.database),
      m_uploads(m_ioc, m_config.database),
      m_users(m_ioc, m_config.database),
      m_license_watcher(m_etcd),
      m_limits(m_license_watcher),
      m_server(m_config.server,
               std::make_unique<handler>(
                   command_factory(m_ioc, *m_dedupe, m_directory, m_uploads,
                                   m_config, m_gdv, m_limits, m_users,
                                   m_license_watcher),
                   http::request_factory(m_users),
                   std::make_unique<policy::module>(m_directory),
                   std::make_unique<cors::module>(cors::config{}, m_directory)),
               m_ioc),
      m_gc(m_ioc, m_directory, m_gdv) {

    co_spawn(m_ioc, update_limits(m_directory, m_limits).start_trace(),
             [](std::exception_ptr e) {
                 try {
                     std::rethrow_exception(e);
                 } catch (const std::exception& e) {
                     LOG_ERROR()
                         << "metrics monitor stopped working: " << e.what();
                 }
             });
}

void service::run() {
    metric<entrypoint_original_data_volume_gauge, byte, int64_t>::
        register_gauge_callback(
            [this]() { return m_limits.get_storage_size(); },
            [this]() {
                auto label =
                    m_license_watcher.get_license()->to_key_value_iterable();
                label.push_back({"service_id", std::to_string(m_service_id)});
                return label;
            });

    auto g = scope_guard([]() {
        metric<entrypoint_original_data_volume_gauge, byte,
               int64_t>::remove_gauge_callback();
    });

    m_service_registry.register_service(m_server.get_server_config());
    m_server.run();
}

void service::stop() {
    LOG_INFO() << "stopping entrypoint " << m_service_id;

    m_server.stop();
}

} // namespace uh::cluster::ep
