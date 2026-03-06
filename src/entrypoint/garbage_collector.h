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

#include <boost/asio.hpp>
#include <common/coroutines/coro_util.h>
#include <common/types/common_types.h>
#include <entrypoint/directory.h>
#include <storage/global/data_view.h>

namespace uh::cluster::ep {

class garbage_collector {
public:
    garbage_collector(boost::asio::io_context& ctx, directory& dir,
                      storage::global::global_data_view& gdv);

private:
    static constexpr auto POLL_INTERVALL = std::chrono::seconds(5);
    static constexpr const char* EP_GC_INITIAL_CONTEXT_NAME =
        "ep-garbe-collector";
    coro<void> collect();

    directory& m_dir;
    storage::global::global_data_view& m_gdv;
    scoped_task m_task;
};

} // namespace uh::cluster::ep
