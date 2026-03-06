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

#include "remote_storage.h"

namespace uh::cluster {

template <>
std::shared_ptr<storage_interface>
service_factory<storage_interface>::make_remote_service(
    const std::string& hostname, uint16_t port) {
    return std::make_shared<remote_storage>(
        client(m_ioc, hostname, port, m_connections));
}

} // namespace uh::cluster
