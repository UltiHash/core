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

#include <string>
#include <typeinfo>

#ifdef __GNUG__
#include <cxxabi.h>
#endif

namespace uh::cluster {

template <typename T> std::string class_name() {
#ifdef __GNUG__
    int status;
    char* buffer = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
    std::string name = buffer;
    free(buffer);
#else
    std::string name = typeid(T).name();
#endif

    return name;
}

} // namespace uh::cluster
