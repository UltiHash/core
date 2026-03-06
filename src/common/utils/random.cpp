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

#include "common/utils/random.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace uh::cluster {

// ---------------------------------------------------------------------

std::string random_string(std::size_t length, const std::string& chars) {
    thread_local static std::mt19937 rg{std::random_device{}()};
    thread_local static std::uniform_int_distribution<std::string::size_type>
        pick(0, chars.size());

    std::string s;
    s.reserve(length);
    while (s.size() < length) {
        s += 97 + chars[pick(rg)] % 25;
    }

    return s;
}

std::string generate_unique_id() {
    boost::uuids::random_generator gen;
    return boost::uuids::to_string(gen());
}

// ---------------------------------------------------------------------

} // namespace uh::cluster
