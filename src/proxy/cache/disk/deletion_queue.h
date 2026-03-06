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

#include <proxy/cache/cache.h>

#include <mutex>
#include <queue>
#include <shared_mutex>

// TODO: See if we can use boost::lockfree::queue here.

namespace uh::cluster::proxy::cache::disk {

template <typename Key, EntryType Entry> class deletion_queue {
public:
    void push(std::shared_ptr<Entry> e) {
        std::unique_lock lock(m_mutex);
        m_queue.push(e);
        m_current_size += e->data_size();
    }

    void push(std::vector<std::shared_ptr<Entry>> ve) {
        std::unique_lock lock(m_mutex);
        for (auto& e : ve) {
            m_queue.push(e);
            m_current_size += e->data_size();
        }
    }

    std::vector<std::shared_ptr<Entry>> pop(std::size_t size) {
        std::unique_lock lock(m_mutex);
        std::vector<std::shared_ptr<Entry>> ret;
        while (!m_queue.empty() && (size != 0)) {
            auto e = m_queue.front();
            m_queue.pop();
            size = (size > e->data_size()) ? size - e->data_size() : 0;
            m_current_size -= e->data_size();
            ret.push_back(std::move(e));
        }
        return ret;
    }

    std::size_t data_size() const {
        std::shared_lock lock(m_mutex);
        return m_current_size;
    }

private:
    mutable std::shared_mutex m_mutex;

    std::queue<std::shared_ptr<Entry>> m_queue;
    std::size_t m_current_size = 0;
};

} // namespace uh::cluster::proxy::cache::disk
