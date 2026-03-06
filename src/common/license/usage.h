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

#include <common/db/db.h>
#include <common/types/common_types.h>

namespace uh::cluster {

class usage {
public:
    usage(boost::asio::io_context& ioc, const db::config& cfg)
        : m_db(connection_factory(ioc, cfg, cfg.directory),
               cfg.directory.count) {}

    coro<std::size_t> get_usage_for_interval(const utc_time& interval_infimum,
                                             const utc_time& interval_supremum);

private:
    pool<db::connection> m_db;
};
} // namespace uh::cluster
