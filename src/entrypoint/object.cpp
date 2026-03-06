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

#include "object.h"


namespace uh::cluster::ep {

object_state to_object_state(const std::string& s) {
    if (s == "Normal") {
        return object_state::normal;
    }

    if (s == "Deleted") {
        return object_state::deleted;
    }

    if (s == "Collected") {
        return object_state::collected;
    }

    throw std::runtime_error("unsupported object state: " + s);
}

std::string to_string(object_state os) {
    switch (os) {
        case object_state::normal: return "Normal";
        case object_state::deleted: return "Deleted";
        case object_state::collected: return "Collected";
    }

    throw std::runtime_error("unsupport object state");
}

} // namespace uh::cluster::ep
