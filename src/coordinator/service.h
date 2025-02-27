#pragma once

#include "config.h"

#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/telemetry/log.h>
#include <common/utils/common.h>
#include <common/utils/io_context_runner.h>
#include <common/utils/strings.h>
#include <config/configuration.h>

#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>

#include <common/license/backend_client.h>
#include <common/license/license_updater.h>
#include <common/license/usage_updater.h>

#include <storage/ec_groups/ec_group_maintainer.h>
#include <storage/interfaces/global_data_view.h>

namespace uh::cluster::coordinator {

class service {
public:
    service(const service_config& service, const coordinator_config& cc)
        : m_etcd{service.etcd_config},
          m_ioc(cc.thread_count),

          m_ioc_runner(m_ioc, cc.thread_count),
          m_ec_maintainer(m_ioc, 1, 0, m_etcd, true),

          m_storage_maintainer(m_etcd, client_factory(m_ioc, 1)),
          m_usage(m_ioc, cc.database_config) {

        if (cc.license) {
            LOG_INFO() << "using license from UH_LICENSE";
            m_license_updater.emplace(
                m_ioc, m_etcd, pseudo_backend_client(cc.license.to_string()));
            boost::asio::co_spawn(m_ioc, m_license_updater->update(),
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
                m_ioc, m_license_updater->periodic_update(LICENSE_FETCH_PERIOD),
                boost::asio::detached);

            m_usage_updater.emplace(m_ioc, m_usage, *m_license_updater,
                                    default_backend_client(bc.backend_host,
                                                           bc.customer_id,
                                                           bc.access_token));
        }
        m_storage_maintainer.add_monitor(m_ec_maintainer);
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
        m_storage_maintainer.remove_monitor(m_ec_maintainer);
    }

private:
    etcd_manager m_etcd;
    boost::asio::io_context m_ioc;
    std::condition_variable m_cv;
    std::mutex m_mutex;
    bool m_stopped = false;

    io_context_runner m_ioc_runner;

    ec_group_maintainer m_ec_maintainer;
    service_maintainer<client, client_factory, STORAGE_SERVICE>
        m_storage_maintainer;

    usage m_usage;
    std::optional<license_updater> m_license_updater;
    std::optional<usage_updater> m_usage_updater;
};
} // namespace uh::cluster::coordinator
