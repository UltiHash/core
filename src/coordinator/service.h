#pragma once

#include "config.h"

#include <common/etcd/ec_groups/ec_group_maintainer.h>
#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/telemetry/log.h>
#include <common/utils/common.h>
#include <common/utils/io_context_runner.h>
#include <common/utils/strings.h>
#include <config/configuration.h>

#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>

#include <common/license/payg/fetch.h>
#include <common/license/payg/updater.h>

namespace uh::cluster::coordinator {

class service {
public:
    service(const service_config& service, const coordinator_config& cc)
        : m_etcd{service.etcd_config},
          m_ioc(cc.thread_count),

          m_ioc_runner(m_ioc, cc.thread_count),
          m_ec_maintainer(m_ioc, 1, 0, m_etcd, true),

          m_storage_maintainer(
              m_etcd, service_factory<storage_interface>(m_ioc, 1, nullptr)),

          m_payg_updater{
              m_ioc,
              m_etcd, //

              // TODO: How should we finalize this part before server is
              // implemented?

              // TODO: We can use codes below and read infomation from
              // environment variables using std::getenv(), UH_CUSTOMER_ID,
              // UH_ACCESS_TOKEN.

              // [&]() -> coro<std::string> {
              //     const std::string url{"<url>/v1/license"};
              //     const std::string username{""};
              //     const std::string password{""};
              //     co_return co_await fetch_response_body(
              //         m_ioc, url, username, password);
              // }

              [&]() -> coro<std::string> {
                  static constexpr const char* json_literal = R"({
                                 "customer_id": "big corp xy",
                                 "license_type": "freemium",
                                 "storage_cap": 10240,
                                 "ec": {
                                     "enabled": true,
                                     "max_group_size": 10
                                 },
                                 "replication": {
                                     "enabled": true,
                                     "max_replicas": 3
                                 },
                                 "signature":
                                 "yg2DNf6iej5np/rQuM4mkp1xzByxxV6vHmHjrbimLyNndL+biWhajraNcp88mXB6iNy/EQ5Izx8H6Q7mggpxBg=="
                             })";
                  co_return json_literal;
              } //
          }     //
    {

        m_storage_maintainer.add_monitor(m_ec_maintainer);
    }

    void run() {
        LOG_INFO() << "running coordinator service";

        boost::asio::co_spawn(
            m_ioc, m_payg_updater.periodic_update(LICENSE_FETCH_PERIOD),
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

    payg_updater m_payg_updater;
};
} // namespace uh::cluster::coordinator
