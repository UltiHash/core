#pragma once

#include "common/etcd/ec_groups/ec_get_handler.h"
#include "common/etcd/ec_groups/ec_group_maintainer.h"
#include "common/etcd/service_discovery/service_maintainer.h"
#include "common/utils/io_context_runner.h"
#include "config.h"
#include "config/configuration.h"

namespace uh::cluster::recovery {

class service {
public:
    service(const service_config& service, const recovery_config& sc)
        : m_ioc(sc.thread_count),
          m_ioc_runner(m_ioc, sc.thread_count),
          m_etcd{service.etcd_config} {}

    void run() {
        LOG_INFO() << "running recovery service";
        while (!m_stopped) {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [this] { return m_stopped; });
        }
    }

    void stop() {
        LOG_INFO() << "stopping recovery service";
        {
            std::lock_guard lock(m_mutex);
            m_stopped = true;
        }
        m_cv.notify_all();

        m_etcd.stop();
        m_ioc_runner.stop();
        m_ioc.stop();
    }

private:
    boost::asio::io_context m_ioc;
    io_context_runner m_ioc_runner;
    etcd_manager m_etcd;
    std::condition_variable m_cv;
    std::mutex m_mutex;
    bool m_stopped = false;
};

} // namespace uh::cluster::recovery
