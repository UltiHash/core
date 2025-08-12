#pragma once

#include <common/etcd/namespace.h>
#include <common/etcd/subscriber.h>
#include <common/etcd/utils.h>
#include <common/license/license.h>
#include <common/telemetry/log.h>

namespace uh::cluster {

class license_watcher {
public:
    using callback_t = value_observer<license>::callback_t;
    license_watcher(etcd_manager& etcd, callback_t callback = nullptr)
        : m_license{etcd_license_key, callback},
          m_subscriber{
              "license_subscriber", etcd, etcd_license_key, {m_license}} {}
    std::shared_ptr<license> get_license() { return m_license.get(); }

private:
    value_observer<license> m_license;
    subscriber m_subscriber;
};

} // namespace uh::cluster
