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

#include "common/utils/pool.h"
#include "config.h"
#include "connection.h"

namespace uh::cluster::db {

/**
 * Create a connection factory that can be passed to `uh::cluster::pool`.
 */
inline auto connection_factory(boost::asio::io_context& ioc, const config& cfg,
                               const config::database& db_cfg) {

    connstr cs(cfg, db_cfg.dbname);

    return [cs, &ioc]() {
        LOG_INFO() << "connecting to " << cs;

        auto conn = std::make_unique<connection>(ioc, cs);
        auto row = conn->raw_exec("SELECT version();");
        LOG_INFO() << "connected to `" << cs << "`: " << *row->string_view(0);
        return conn;
    };
}

} // namespace uh::cluster::db
