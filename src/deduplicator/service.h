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

#include <common/etcd/registry/service_id.h>
#include <common/etcd/registry/service_registry.h>
#include <common/network/server.h>
#include <common/telemetry/log.h>
#include <config.h>
#include <storage/global/data_view.h>
#include <storage/group/externals.h>
#include <storage/interfaces/remote_storage.h>
#include <storage/service.h>

#include "handler.h"

#include <functional>
#include <utility>

namespace uh::cluster::deduplicator {

class service {
public:
    explicit service(boost::asio::io_context& ioc, const service_config& sc,
                     const deduplicator_config& config)
        : m_etcd{sc.etcd_config},
          m_service_id(get_service_id(m_etcd,
                                      get_service_string(DEDUPLICATOR_SERVICE),
                                      config.working_dir)),
          m_gdv{ioc, m_etcd, config.global_data_view},
          m_cache(ioc, m_gdv, config.global_data_view.read_cache_capacity_l2),
          m_deduplicator(
              std::make_shared<local_deduplicator>(config, m_gdv, m_cache)),
          m_server(config.server, std::make_unique<handler>(*m_deduplicator),
                   ioc),
          m_service_registry(m_etcd,
                             ns::root.deduplicator.hostports[m_service_id],
                             config.server.port) {}

private:
    etcd_manager m_etcd;
    std::size_t m_service_id;

    storage::global::global_data_view m_gdv;
    storage::global::cache m_cache;

    std::shared_ptr<local_deduplicator> m_deduplicator;
    server m_server;
    service_registry m_service_registry;
};
} // namespace uh::cluster::deduplicator
