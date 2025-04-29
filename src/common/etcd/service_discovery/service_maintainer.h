#pragma once

#include <common/etcd/service_discovery/service_observer.h>
#include <common/etcd/subscriber/subscriber.h>
#include <common/etcd/utils.h>
#include <common/service_interfaces/service_factory.h>
#include <common/telemetry/log.h>

namespace uh::cluster {

template <typename service_interface> class service_maintainer {

public:
    service_maintainer(
        etcd_manager& etcd, const std::string& prefix,
        service_factory<service_interface> service_factory,
        std::initializer_list<
            std::reference_wrapper<service_observer<service_interface>>>
            observers)
        : m_hostports_observer{prefix, std::move(service_factory), observers},
          m_subscriber{etcd, prefix, {m_hostports_observer}} {}

    [[nodiscard]] std::size_t size() const noexcept {
        return m_hostports_observer.size();
    }
    hostports_observer<service_interface> m_hostports_observer;
    subscriber m_subscriber;
};

} // namespace uh::cluster
