//
// Created by massi on 8/1/24.
//

#ifndef MAINTAINER_MONITOR_H
#define MAINTAINER_MONITOR_H
#include "../namespace.h"

namespace uh::cluster {

template <typename service_interface> class service_maintainer;

template <typename service_interface> struct service_monitor {

    friend service_maintainer<service_interface>;

private:
    virtual void add_attribute(const std::shared_ptr<service_interface>&,
                               etcd_service_attributes, const std::string&) {}
    virtual void remove_attribute(const std::shared_ptr<service_interface>&,
                                  etcd_service_attributes) {}
    virtual void add_client(size_t, const std::shared_ptr<service_interface>&) {
    }
    virtual void remove_client(size_t,
                               const std::shared_ptr<service_interface>&) {}

    virtual void
    add_local_client(const std::shared_ptr<service_interface>& client) {
        m_local_service = client;
    }

protected:
    virtual ~service_monitor() = default;

    std::shared_ptr<service_interface> m_local_service;

    static constexpr std::size_t m_timeout_s = 10;
};
} // namespace uh::cluster

#endif // MAINTAINER_MONITOR_H
