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

#include <common/network/server.h>
#include <common/etcd/service.h>
#include <storage/global/data_view.h>
#include <proxy/cache/disk/manager.h>
#include "config.h"

#include <boost/asio.hpp>


namespace uh::cluster::proxy {

class service {
public:
    service(boost::asio::io_context& ioc, const service_config& sc, const config& c);

private:
    using manager = cache::disk::manager;

    boost::asio::io_context& m_ioc;
    etcd_manager m_etcd;
    std::unique_ptr<storage::data_view> m_dv;
    manager m_mgr;
    server m_server;
};

} // namespace uh::cluster::proxy
