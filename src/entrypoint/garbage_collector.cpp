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

#include "garbage_collector.h"

namespace uh::cluster::ep {

garbage_collector::garbage_collector(boost::asio::io_context& ioc,
                                     directory& dir,
                                     storage::global::global_data_view& gdv)
    : m_dir(dir),
      m_gdv(gdv),
      m_task{"garbage collector", ioc, collect().start_trace()} {}

coro<void> garbage_collector::collect() {
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);

    auto state = co_await boost::asio::this_coro::cancellation_state;
    while (state.cancelled() == boost::asio::cancellation_type::none) {
        auto to_delete = co_await m_dir.next_deleted();
        if (!to_delete) {
            co_await m_dir.clear_buckets();
            timer.expires_after(POLL_INTERVALL);
            co_await timer.async_wait(boost::asio::use_awaitable);
            continue;
        }

        try {
            auto freed = co_await m_gdv.unlink(to_delete->addr);
            LOG_DEBUG() << "reclaimed " << freed
                        << " bytes of data by disposing object "
                        << to_delete->id;
        } catch (const std::exception& e) {
            LOG_WARN() << "deleting of object " << to_delete->id
                       << " failed: " << e.what();
        }

        co_await m_dir.remove_object(to_delete->id);
    }
}

} // namespace uh::cluster::ep
