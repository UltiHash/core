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
