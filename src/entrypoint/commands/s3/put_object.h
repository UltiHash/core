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
#include <entrypoint/limits.h>
#include <storage/global/data_view.h>
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class put_object : public command {
public:
    put_object(limits&, directory&, storage::global::global_data_view&,
               deduplicator_interface&);

    static bool can_handle(const ep::http::request& req);

    coro<void> validate(const ep::http::request& req) override;

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    storage::global::global_data_view& m_gdv;
    limits& m_limits;
    deduplicator_interface& m_dedupe;
};

} // namespace uh::cluster
