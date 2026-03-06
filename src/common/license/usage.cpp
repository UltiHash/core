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

#include "usage.h"
#include <format>

namespace uh::cluster {

coro<std::size_t>
usage::get_usage_for_interval(const utc_time& interval_infimum,
                              const utc_time& interval_supremum) {
    auto handle = co_await m_db.get();

    auto row =
        co_await handle->execb("select uh_compute_usage($1, $2)",
                               std::format("{0:%F %T}", interval_infimum),
                               std::format("{0:%F %T}", interval_supremum));

    if (!row) {
        co_return 0;
    }

    co_return *row->number(0);
}

} // namespace uh::cluster
