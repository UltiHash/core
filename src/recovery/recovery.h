#ifndef CORE_RECOVERY_H
#define CORE_RECOVERY_H

#include "common/etcd/ec_groups/ec_get_handler.h"
#include "common/etcd/ec_groups/ec_group_maintainer.h"
#include "common/utils/io_context_runner.h"
#include "config.h"
#include "config/configuration.h"

namespace uh::cluster {

template <size_t num> struct test_desc {
    ~test_desc() { std::cout << "test desc " << num << std::endl; }
};

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
        {
            std::lock_guard lock(m_mutex);
            m_stopped = true;
        }
        m_cv.notify_all();
        m_ioc_runner.stop();
    }

private:
    etcd::SyncClient m_etcd_client;

    boost::asio::io_context m_ioc;
    std::condition_variable m_cv;
    std::mutex m_mutex;
    bool m_stopped = false;
    test_desc<4> t4;

    io_context_runner m_ioc_runner;
    test_desc<3> t3;
    service_maintainer<storage_interface> m_storage_maintainer;
    test_desc<2> t2;
    ec_group_maintainer m_ec_maintainer;
    test_desc<1> t1;
    ec_get_handler m_getter;
    test_desc<0> t0;
};
} // end namespace uh::cluster
#endif // CORE_RECOVERY_H
