#pragma once

#include "common/etcd/ec_groups/ec_get_handler.h"
#include "config.h"

#include <common/etcd/ec_groups/ec_group_maintainer.h>
#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/telemetry/log.h>
#include <common/utils/io_context_runner.h>
#include <config/configuration.h>

#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>

#include <common/license/payg_updater.h>
#include <common/license/test.h>
#include <common/utils/strings.h>

namespace uh::cluster::coordinator {

class service {
public:
    service(const service_config& service, const coordinator_config& cc)
        : m_etcd{service.etcd_config},
          m_ioc(cc.thread_count),

          m_ioc_runner(m_ioc, cc.thread_count),
          m_ec_maintainer(m_ioc, 1, 0, m_etcd, true),

          m_storage_maintainer(
              m_etcd, service_factory<storage_interface>(m_ioc, 1, nullptr)) {

        m_storage_maintainer.add_monitor(m_ec_maintainer);
    }

    void run() {
        LOG_INFO() << "running coordinator service";

        boost::asio::co_spawn(m_ioc,
                              // TODO: replace 3s with 1h
                              periodic_executor(m_ioc, 3s,
                                                [&]() -> coro<void> {
                                                    co_return co_await publish(
                                                        m_ioc, m_etcd);
                                                }),
                              boost::asio::detached);

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
    service_maintainer<storage_interface> m_storage_maintainer;
};
} // namespace uh::cluster::coordinator
