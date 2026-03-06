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

#include "common/etcd/service_discovery/service_observer.h"

#include <condition_variable>
#include <set>

namespace uh::cluster {

template <typename service_interface>
class service_load_balancer : public service_observer<service_interface> {

public:
    explicit service_load_balancer() {}

    void add_client(size_t id,
                    std::shared_ptr<service_interface> service) override {
        std::lock_guard l(m_mutex);
        m_services.emplace(id, service);
        m_cv.notify_one();
    }

    void remove_client(size_t id) override {
        std::lock_guard l(m_mutex);

        auto it = m_services.find(id);
        if (it == m_services.end()) {
            return;
        }
        if (it == m_robin_index) {
            m_robin_index = m_services.erase(it);
        } else {
            m_services.erase(it);
        }
    }

    std::pair<std::size_t, std::shared_ptr<service_interface>> get() {

        std::unique_lock lk(m_mutex);

        if (!m_cv.wait_for(lk, time_settings::instance().service_get_timeout,
                           [this] { return !empty(); })) {
            throw std::runtime_error(
                "[load balancer] timeout waiting for any " +
                get_service_string(service_interface::service_role) +
                " client");
        }

        if (m_robin_index == m_services.cend()) {
            m_robin_index = m_services.cbegin();
        }

        auto rv = std::pair<std::size_t, std::shared_ptr<service_interface>>(
            m_robin_index->first, m_robin_index->second);
        ++m_robin_index;

        return rv;
    }

    [[nodiscard]] bool empty() const noexcept { return m_services.empty(); }

    [[nodiscard]] size_t size() const noexcept { return m_services.size(); }

protected:
    std::condition_variable m_cv;
    std::mutex m_mutex;
    std::map<std::size_t, std::shared_ptr<service_interface>> m_services;

private:
    typename std::map<std::size_t,
                      std::shared_ptr<service_interface>>::const_iterator
        m_robin_index = m_services.cend();
};

} // namespace uh::cluster
