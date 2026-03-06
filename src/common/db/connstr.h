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

#include "config.h"
#include <ostream>

namespace uh::cluster::db {

class connstr {
public:
    connstr(const config& cfg, const std::string& dbname = "");

    const std::string& get() const;
    std::string printable() const;

    operator const char*() const;

    connstr& use(std::string dbname);

private:
    config m_cfg;
    std::string m_dbname;
    std::string m_connstr;
};

std::ostream& operator<<(std::ostream& out, const connstr& c);

} // namespace uh::cluster::db
