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
#include <entrypoint/directory.h>
#include <entrypoint/multipart_state.h>
#include <storage/global/data_view.h>
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class multipart : public command {
public:
    multipart(deduplicator_interface&, storage::global::global_data_view&,
              multipart_state&);

    static bool can_handle(const ep::http::request& req);

    coro<void> validate(const ep::http::request& req) override;

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    deduplicator_interface& m_dedupe;
    storage::global::global_data_view& m_gdv;
    multipart_state& m_uploads;
};

} // namespace uh::cluster
