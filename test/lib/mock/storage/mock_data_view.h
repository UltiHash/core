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

#include <storage/global/data_view.h>

#include "mock_data_store.h"

namespace uh::cluster {

class mock_data_view : public storage::data_view {
public:
    explicit mock_data_view(mock_data_store& storage);

    coro<address> write(std::span<const char> data,
                        const std::vector<std::size_t>& offsets) override;
    coro<shared_buffer<>> read(const uint128_t& pointer, size_t size) override;
    coro<std::size_t> read_address(const address& addr,
                                   std::span<char> buffer) override;
    [[nodiscard]] coro<address> link(const address& addr) override;
    coro<std::size_t> unlink(const address& addr) override;
    coro<std::size_t> get_used_space() override;

    ~mock_data_view() noexcept = default;

private:
    mock_data_store& m_storage;
};

} // namespace uh::cluster
