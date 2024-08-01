//
// Created by massi on 8/1/24.
//

#ifndef MAINTAINER_MONITOR_H
#define MAINTAINER_MONITOR_H
#include "../namespace.h"

namespace uh::cluster {

template <typename service_interface> class service_maintainer;

template <typename service_interface> struct maintainer_monitor {

    friend service_maintainer<service_interface>;

private:
    virtual void add_attribute(const std::shared_ptr<service_interface>&,
                               etcd_service_attributes, const std::string&) {}
    virtual void remove_attribute(const std::shared_ptr<service_interface>&,
                                  etcd_service_attributes) {}
    virtual void add_client(size_t, const std::shared_ptr<service_interface>&) {
    }
    virtual void remove_client(const std::shared_ptr<service_interface>&) {}

    virtual void
    add_local_client(const std::shared_ptr<service_interface>& client) {
        m_local_service = client;
    }

    void set_sync_vars(std::mutex& m, std::condition_variable& cv) {
        m_mutex.emplace(m);
        m_cv.emplace(cv);
    }

protected:
    virtual ~maintainer_monitor() = default;

    optref<std::mutex> m_mutex;
    optref<std::condition_variable> m_cv;
    std::shared_ptr<service_interface> m_local_service;

    static constexpr std::size_t m_timeout_s = 10;
};
} // namespace uh::cluster

#endif // MAINTAINER_MONITOR_H
