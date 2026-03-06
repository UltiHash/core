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

#include "common/types/common_types.h"
#include "common/types/scoped_buffer.h"
#include "common/utils/common.h"

namespace uh::cluster {
struct storage_interface {
    virtual coro<allocation_t>
    allocate(std::size_t size, std::size_t alignment = DEFAULT_PAGE_SIZE) = 0;

    virtual coro<void> write(allocation_t allocation,
                             const std::vector<std::span<const char>>& buffers,
                             const std::vector<refcount_t>& refcounts) = 0;

    virtual coro<shared_buffer<>> read(const storage_pointer& pointer,
                                       size_t size) = 0;

    virtual coro<void> read_address(const storage_address& addr,
                                    std::span<char> buffer,
                                    const std::vector<size_t>& offsets) = 0;

    virtual coro<std::vector<refcount_t>>
    link(const std::vector<refcount_t>& refcounts) = 0;

    virtual coro<std::size_t>
    unlink(const std::vector<refcount_t>& refcounts) = 0;

    virtual coro<std::vector<refcount_t>>
    get_refcounts(const std::vector<std::size_t>& stripe_ids) = 0;

    virtual coro<std::size_t> get_used_space() = 0;

    virtual ~storage_interface() noexcept = default;

    static constexpr role service_role = STORAGE_SERVICE;
};

} // namespace uh::cluster
