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

#include <common/types/address.h>
#include <common/types/common_types.h>
#include <common/utils/common.h>
#include <span>

namespace uh::cluster {

struct data_store_config {
    size_t max_file_size = 1_GiB;
    size_t max_data_store_size = 1_PiB;
    size_t page_size = DEFAULT_PAGE_SIZE;
};

struct data_store {
    virtual allocation_t allocate(size_t size, std::size_t alignment = 1) = 0;

    virtual void write(const allocation_t allocation,
                       const std::vector<std::span<const char>>& buffers,
                       const std::vector<refcount_t>& refcounts = {}) = 0;

    virtual std::size_t read(const storage_pointer local_pointer,
                             std::span<char> buffer) = 0;

    virtual std::vector<refcount_t>
    link(const std::vector<refcount_t>& refcounts) = 0;

    virtual std::size_t unlink(const std::vector<refcount_t>& refcounts) = 0;

    virtual std::vector<refcount_t>
    get_refcounts(const std::vector<std::size_t>& stripe_ids) = 0;

    virtual std::size_t get_used_space() const noexcept = 0;

    virtual std::size_t get_available_space() const noexcept = 0;

    virtual ~data_store() = default;
};

} // end namespace uh::cluster
