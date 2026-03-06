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

#include "dedupe_array.h"
#include "remote_deduplicator.h"

namespace uh::cluster {

dedupe_array::dedupe_array(boost::asio::io_context& ioc, etcd_manager& etcd,
                           std::size_t connections)
    : m_etcd(etcd),
      m_dedupe_maintainer(
          m_etcd, ns::root.deduplicator.hostports,
          service_factory<deduplicator_interface>(ioc, connections),
          {m_dedupe_load_balancer}) {}

coro<dedupe_response> dedupe_array::deduplicate(std::string_view data) {
    auto [dedup_id, client] = m_dedupe_load_balancer.get();
    co_return co_await client->deduplicate(data);
}

} // namespace uh::cluster
