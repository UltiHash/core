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

#include <common/etcd/impl/subscriber_observer.h>
#include <common/etcd/reader.h>

#include <common/etcd/utils.h>
#include <common/telemetry/log.h>

namespace uh::cluster {

/*
 * Subscriber manages multiple keys by using recursive watch.
 */
class subscriber {
public:
    using callback_t = std::function<void()>;

    subscriber(
        std::string name, etcd_manager& etcd, const std::string& key,
        std::initializer_list<std::reference_wrapper<subscriber_observer>>
            observers,
        callback_t callback = nullptr)
        : m_reader{name, etcd, key, observers, std::move(callback)} {

        m_wg = etcd.watch(
            key,
            [this](etcd_manager::response resp) { m_reader.on_watch(resp); },
            m_reader.get_index() + 1);
    }

private:
    reader m_reader;
    etcd_manager::watch_guard m_wg;
};

} // namespace uh::cluster
