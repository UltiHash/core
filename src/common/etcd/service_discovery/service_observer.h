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

#include "common/etcd/namespace.h"

namespace uh::cluster {

template <typename service_interface> class service_observer {

public:
    virtual void add_client(size_t, std::shared_ptr<service_interface>) = 0;
    virtual void remove_client(size_t) = 0;

    virtual ~service_observer() = default;
};
} // namespace uh::cluster
