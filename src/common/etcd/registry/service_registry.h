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

#include "common/etcd/utils.h"
#include "common/network/server.h"
#include <magic_enum/magic_enum.hpp>
#include <string>

namespace uh::cluster {

class service_registry {

public:
    service_registry(etcd_manager& etcd, const std::string& key, uint16_t port);
    ~service_registry();

protected:
    etcd_manager& m_etcd;

private:
    std::string m_key;
};

} // namespace uh::cluster
