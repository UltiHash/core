/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <common/etcd/service_discovery/service_observer.h>
#include <common/etcd/subscriber.h>
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
          m_subscriber{
              "service_maintainer", etcd, prefix, {m_hostports_observer}} {}

    [[nodiscard]] std::size_t size() const noexcept {
        return m_hostports_observer.size();
    }
    hostports_observer<service_interface> m_hostports_observer;
    subscriber m_subscriber;
};

} // namespace uh::cluster
