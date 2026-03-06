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

#include <common/license/license_watcher.h>

#include <atomic>

namespace uh::cluster {

class limits {
public:
    limits(license_watcher& watcher);

    /**
     * Set storage size without checking.
     */
    void set_storage_size(std::size_t size);

    std::size_t get_storage_size() const;
    /**
     * Check internal storage size and increment the counter.
     */
    void check_storage_size(std::size_t increment);

    // warn about a nearly reached size limit at this percentage
    static constexpr unsigned SIZE_LIMIT_WARNING_PERCENTAGE = 80;
    // number of files to upload between two warnings
    static constexpr unsigned SIZE_LIMIT_WARNING_INTERVAL = 100;

private:
    license_watcher& m_watcher;
    std::atomic<std::size_t> m_data_storage_size;
    unsigned m_warn_counter = SIZE_LIMIT_WARNING_INTERVAL;
};

} // namespace uh::cluster
