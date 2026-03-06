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

#include "common/types/address.h"
#include "common/types/big_int.h"
#include "common/types/common_types.h"
#include "common/types/scoped_buffer.h"

namespace uh::cluster::storage {

class data_view {

public:
    virtual coro<address> write(std::span<const char> data,
                                const std::vector<std::size_t>& offsets) = 0;

    virtual coro<shared_buffer<>> read(const pointer& pointer, size_t size) = 0;

    virtual coro<std::size_t> read_address(const address& addr,
                                           std::span<char> buffer) = 0;

    [[nodiscard]] virtual coro<address> link(const address& addr) = 0;

    virtual coro<std::size_t> unlink(const address& addr) = 0;

    virtual coro<std::size_t> get_used_space() = 0;

    virtual ~data_view() noexcept = default;
};

} // namespace uh::cluster::storage
