#pragma once

#include "common/etcd/ec_groups/ec_get_handler.h"
#include "common/etcd/ec_groups/ec_group_maintainer.h"
#include "common/etcd/service_discovery/service_maintainer.h"
#include "common/telemetry/log.h"
#include "common/utils/io_context_runner.h"
#include "config.h"
#include "config/configuration.h"
#include "fetch.h"

#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>

namespace uh::cluster::coordinator {

coro<void> license_handler(boost::asio::io_context& io_context) {
    while (true) {
        boost::asio::steady_timer timer(io_context, 1s);
        co_await timer.async_wait(boost::asio::use_awaitable);
        try {
            auto license =
                co_await fetch_response_body(io_context, "example.com");
            LOG_INFO() << "license: " << license;

            // put license to etcd?
        } catch (...) {
        }
    }
}

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

        boost::asio::co_spawn(m_ioc, license_handler(m_ioc),
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
