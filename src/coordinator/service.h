#pragma once

#include "config.h"

#include <common/etcd/service.h>
#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/telemetry/log.h>
#include <common/utils/common.h>
#include <common/utils/io_context_runner.h>
#include <common/utils/strings.h>
#include <storage/interfaces/remote_storage.h>

#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>

#include <common/license/backend_client.h>
#include <common/license/license_updater.h>
#include <common/license/usage_updater.h>
#include <ranges>

namespace uh::cluster::coordinator {

class service {
public:
    service(const service_config& service, const coordinator_config& cc)
        : m_etcd{service.etcd_config},
          m_ioc(cc.thread_count),
          m_ioc_runner(m_ioc, cc.thread_count),
          m_usage{m_ioc, cc.database_config} {

        if (cc.license) {
            LOG_INFO() << "using license from UH_LICENSE";
            m_license_updater.emplace(
                m_ioc, m_etcd, pseudo_backend_client(cc.license.to_string()));

            boost::asio::co_spawn(m_ioc,
                                  m_license_updater->update().start_trace(),
                                  boost::asio::detached);
        } else {
            LOG_INFO() << "using license from licensing host "
                       << cc.backend_config.backend_host;
            const auto& bc = cc.backend_config;
            m_license_updater.emplace(m_ioc, m_etcd,
                                      default_backend_client(bc.backend_host,
                                                             bc.customer_id,
                                                             bc.access_token));
            boost::asio::co_spawn(
                m_ioc,
                m_license_updater->periodic_update(LICENSE_FETCH_PERIOD)
                    .start_trace(),
                boost::asio::detached);

            m_usage_updater.emplace(m_ioc, m_usage, *m_license_updater,
                                    default_backend_client(bc.backend_host,
                                                           bc.customer_id,
                                                           bc.access_token));
        }
        publish_configs(m_etcd, cc.storage_groups);
    }

    void run() {
        LOG_INFO() << "running coordinator service";

        while (!m_stopped) {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [this] { return m_stopped; });
        }
    }

    void stop() {
        LOG_INFO() << "stopping coordinator service";
        {
            std::lock_guard lock(m_mutex);
            m_stopped = true;
        }
        m_cv.notify_all();
    }

    static void publish_configs(etcd_manager& etcd,
                                const storage::group_configs& group_configs) {
        for (const auto& cfg : group_configs.configs) {
            etcd.put(ns::root.storage_groups.group_configs[cfg.id],
                     cfg.to_string());
        }
    }

private:
    etcd_manager m_etcd;
    boost::asio::io_context m_ioc;
    std::condition_variable m_cv;
    std::mutex m_mutex;
    bool m_stopped = false;

    io_context_runner m_ioc_runner;

    usage m_usage;
    std::optional<license_updater> m_license_updater;
    std::optional<usage_updater> m_usage_updater;
};
} // namespace uh::cluster::coordinator
