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

#include <common/coroutines/worker_pool.h>
#include <common/service_interfaces/deduplicator_interface.h>
#include <deduplicator/config.h>
#include <deduplicator/dedupe_set/fragment_set.h>
#include <deduplicator/fragmentation.h>
#include <storage/global/cache.h>
#include <storage/global/data_view.h>

namespace uh::cluster {

struct local_deduplicator : public deduplicator_interface {

    local_deduplicator(deduplicator_config config, storage::data_view& storage,
                       storage::global::cache& cache);

    coro<dedupe_response> deduplicate(std::string_view data) override;

private:
    deduplicator_config m_dedupe_conf;
    storage::data_view& m_storage;
    storage::global::cache& m_cache;
    fragment_set m_fragment_set;
    worker_pool m_dedupe_workers;
};
} // namespace uh::cluster
