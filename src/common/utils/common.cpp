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

#include "common.h"

#include <map>

namespace uh::cluster {

static const std::map<uh::cluster::role, std::string> abbreviation_by_role = {
    {uh::cluster::STORAGE_SERVICE, "storage"},
    {uh::cluster::DEDUPLICATOR_SERVICE, "deduplicator"},
    {uh::cluster::ENTRYPOINT_SERVICE, "entrypoint"},
    {uh::cluster::COORDINATOR_SERVICE, "coordinator"}};

const std::string& get_service_string(const role& service_role) {
    if (auto search = abbreviation_by_role.find(service_role);
        search != abbreviation_by_role.end())
        return search->second;
    else
        throw std::invalid_argument("invalid role");
}

} // namespace uh::cluster
