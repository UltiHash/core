#ifndef CORE_RECOVERY_H
#define CORE_RECOVERY_H

#include "common/utils/io_context_runner.h"
#include "config.h"
#include "config/configuration.h"

namespace uh::cluster {

class recovery {
public:
    recovery(const service_config& service, const recovery_config& sc)
        : m_etcd_client(service.etcd_url),
          m_ioc(sc.thread_count),
          m_ioc_runner(m_ioc, sc.thread_count),
          m_storage_maintainer(
              m_etcd_client,
              service_factory<storage_interface>(m_ioc, 1, nullptr)),
          m_ec_maintainer(m_ioc, 1, 0, m_etcd_client),
          m_getter(1, 0) {
        m_storage_maintainer.add_monitor(m_ec_maintainer);
        m_ec_maintainer.add_monitor(m_getter);
    }

    void run() {
        std::unique_lock lock(m_mutex);
        m_cv.wait(lock, [this] { return m_stopped; });
    }

    void stop() {
        LOG_INFO() << "stopping recovery service";
        std::lock_guard lock(m_mutex);
        m_stopped = true;
        m_cv.notify_all();
        m_ioc_runner.stop();
    }

private:
    etcd::SyncClient m_etcd_client;

    boost::asio::io_context m_ioc;
    std::condition_variable m_cv;
    std::mutex m_mutex;
    bool m_stopped = false;
    io_context_runner m_ioc_runner;
    service_maintainer<storage_interface> m_storage_maintainer;
    ec_group_maintainer m_ec_maintainer;
    ec_get_handler m_getter;
};
} // end namespace uh::cluster
#endif // CORE_RECOVERY_H
