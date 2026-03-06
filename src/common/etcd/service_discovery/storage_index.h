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
#include "common/service_interfaces/storage_interface.h"
#include "common/utils/pointer_traits.h"
#include "service_load_balancer.h"

#include <atomic>
#include <ranges>

namespace uh::cluster {

class storage_index : public service_observer<storage_interface> {
public:
    explicit storage_index(std::size_t num_storages)
        : m_services(num_storages) {}

    void add_client(size_t id,
                    std::shared_ptr<storage_interface> service) override {
        m_services.at(id).store(service, std::memory_order_release);
    }
    void remove_client(size_t id) override {
        m_services.at(id).store(nullptr, std::memory_order_release);
    }

    std::shared_ptr<storage_interface> at(std::size_t id) {
        if (id >= m_services.size()) {
            throw std::out_of_range("access wrong index storage");
        }

        auto rv = m_services.at(id).load(std::memory_order_acquire);
        if (rv == nullptr) {
            throw std::runtime_error("storage " + std::to_string(id) +
                                     " is not available");
        }

        return rv;
    }

    auto get() {
        std::vector<std::shared_ptr<storage_interface>> rv;
        rv.reserve(m_services.size());
        for (const auto& atom : m_services) {
            rv.push_back(atom.load(std::memory_order_acquire));
        }
        return rv;
    }

private:
    std::vector<std::atomic<std::shared_ptr<storage_interface>>> m_services;
};

} // namespace uh::cluster
