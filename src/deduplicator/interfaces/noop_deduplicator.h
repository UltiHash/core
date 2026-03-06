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

#include <common/service_interfaces/deduplicator_interface.h>
#include <storage/global/data_view.h>

namespace uh::cluster {

class noop_deduplicator : public deduplicator_interface {
public:
    noop_deduplicator(storage::global::global_data_view& storage);

    coro<dedupe_response> deduplicate(std::string_view data) override;

private:
    storage::global::global_data_view& m_storage;
};

} // namespace uh::cluster
