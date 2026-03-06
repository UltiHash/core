// Copyright 2026 UltiHash Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "noop_deduplicator.h"

namespace uh::cluster {

noop_deduplicator::noop_deduplicator(storage::global::global_data_view& storage)
    : m_storage(storage) {}

coro<dedupe_response> noop_deduplicator::deduplicate(std::string_view data) {
    auto addr = co_await m_storage.write(data, {0});

    co_return dedupe_response{.effective_size = data.size(),
                              .addr = std::move(addr)};
}

} // namespace uh::cluster
