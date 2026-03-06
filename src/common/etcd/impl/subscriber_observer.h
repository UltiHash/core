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

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>
#include <type_traits>

namespace uh::cluster {

struct subscriber_observer {
    virtual bool on_watch(etcd_manager::response resp) = 0;
    virtual ~subscriber_observer() = default;
};

template <typename T>
concept Serializable = requires(const T& t, const std::string& s) {
    { serialize(t) } -> std::convertible_to<std::string>;
    { deserialize<T>(s) } -> std::same_as<T>;
};

} // namespace uh::cluster
