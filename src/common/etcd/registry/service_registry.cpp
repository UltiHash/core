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

#include "service_registry.h"

#include <common/etcd/namespace.h>
#include <common/service_interfaces/hostport.h>
#include <common/utils/common.h>
#include <common/utils/host_utils.h>
#include <common/utils/strings.h>

using namespace boost::asio;

namespace uh::cluster {

service_registry::service_registry(etcd_manager& etcd, const std::string& key,
                                   uint16_t port)
    : m_etcd(etcd),
      m_key{key} {
    LOG_DEBUG() << "Registering service with port: " << port;
    m_etcd.put(m_key, serialize(hostport(get_host(), port)));
}

service_registry::~service_registry() { m_etcd.rm(m_key); }

} // namespace uh::cluster
