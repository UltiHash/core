
#ifndef MAINTAINER_MONITOR_H
#define MAINTAINER_MONITOR_H
#include "common/etcd/namespace.h"

#include <iostream>

namespace uh::cluster {

template <typename service_interface> struct service_monitor {

    virtual void add_attribute(const std::shared_ptr<service_interface>&,
                               etcd_service_attributes, const std::string&) {}
    virtual void remove_attribute(const std::shared_ptr<service_interface>&,
                                  etcd_service_attributes) {}
    virtual void add_client(size_t, const std::shared_ptr<service_interface>&) {
    }
    virtual void remove_client(size_t,
                               const std::shared_ptr<service_interface>&) {}

    virtual ~service_monitor() = default;
};
} // namespace uh::cluster

#endif // MAINTAINER_MONITOR_H
