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

#include <deduplicator/interfaces/local_deduplicator.h>
#include <mock/storage/mock_data_view.h>
#include <util/coroutine.h>
#include <util/temp_directory.h>

#define MAX_DATA_STORE_SIZE_BYTES (4 * MEBI_BYTE)
#define MAX_FILE_SIZE_BYTES (128 * KIBI_BYTE)
#define DATA_STORE_ID 1

namespace uh::cluster {

struct dedupe_fixture : public coro_fixture {
    dedupe_fixture()
        : coro_fixture(2),
          dir{},
          config{.max_file_size = MAX_FILE_SIZE_BYTES,
                 .max_data_store_size = MAX_DATA_STORE_SIZE_BYTES,
                 .page_size = DEFAULT_PAGE_SIZE},
          data_store{config, dir.path().string(), DATA_STORE_ID, 0},
          data_view{data_store},
          cache{m_ioc, data_view, 4000ul},
          dedupe{{}, data_view, cache} {

        auto log_config = log::config{
            .sinks = {log::sink_config{.type = log::sink_type::cout,
                                       .level = boost::log::trivial::fatal,
                                       .service_role = DEDUPLICATOR_SERVICE}}};
        log::init(log_config);
    }

    temp_directory dir;
    data_store_config config;
    mock_data_store data_store;
    mock_data_view data_view;
    storage::global::cache cache;
    local_deduplicator dedupe;
};

} // namespace uh::cluster
