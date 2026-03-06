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

#include <common/etcd/service_discovery/service_load_balancer.h>
#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/etcd/utils.h>
#include <common/service_interfaces/deduplicator_interface.h>

namespace uh::cluster {

class dedupe_array : public deduplicator_interface {
public:
    dedupe_array(boost::asio::io_context& ioc, etcd_manager& etcd,
                 std::size_t connections);

    coro<dedupe_response> deduplicate(std::string_view data) override;

private:
    etcd_manager& m_etcd;
    service_load_balancer<deduplicator_interface> m_dedupe_load_balancer;
    service_maintainer<deduplicator_interface> m_dedupe_maintainer;
};

} // namespace uh::cluster
