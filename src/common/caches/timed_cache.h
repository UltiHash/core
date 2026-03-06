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

#include <chrono>
#include <map>
#include <mutex>

namespace uh::cluster {

/**
 * A cache implementation that stores values under a certain key and ensures
 * that entries are not older than a given duration.
 */
template <typename Key, typename Value> class timed_cache {
public:
    /**
     * @param retention maximum age of entries in the cache
     */
    timed_cache(std::chrono::system_clock::duration retention)
        : m_retention(retention) {}

    std::optional<Value> get(const Key& key) {
        std::unique_lock lock(m_mutex);

        auto entry = m_entries.find(key);
        if (entry == m_entries.end()) {
            return std::nullopt;
        }

        if (entry->second.deadline < std::chrono::system_clock::now()) {
            m_entries.erase(entry);
            return std::nullopt;
        }

        return entry->second.value;
    }

    void put(const Key& key, Value value) {
        std::unique_lock lock(m_mutex);

        auto& entry = m_entries[key];
        entry.value = std::move(value);
        entry.deadline = std::chrono::system_clock::now() + m_retention;
    }

private:
    struct entry {
        std::chrono::time_point<std::chrono::system_clock> deadline;
        Value value;
    };

    std::chrono::system_clock::duration m_retention;
    std::map<Key, entry> m_entries;
    std::mutex m_mutex;
};

} // namespace uh::cluster
