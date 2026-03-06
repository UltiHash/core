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

#include "connstr.h"

namespace uh::cluster::db {

namespace {

std::string mk_url(const config& cfg, const std::string& dbname) {

    std::string rv = "postgresql://";

    if (!cfg.username.empty()) {
        rv += cfg.username;

        if (!cfg.password.empty()) {
            rv += ":" + cfg.password;
        }

        rv += "@";
    }

    rv += cfg.host_port + "/" + dbname;
    return rv;
}

} // namespace

connstr::connstr(const config& cfg, const std::string& dbname)
    : m_cfg(cfg),
      m_dbname(dbname),
      m_connstr(mk_url(m_cfg, m_dbname)) {}

const std::string& connstr::get() const { return m_connstr; }

std::string connstr::printable() const {

    auto cfg = m_cfg;
    if (!cfg.password.empty()) {
        cfg.password = "*****";
    }

    return mk_url(cfg, m_dbname);
}

connstr::operator const char*() const { return m_connstr.c_str(); }

connstr& connstr::use(std::string dbname) {
    m_dbname = std::move(dbname);
    m_connstr = mk_url(m_cfg, m_dbname);
    return *this;
}

std::ostream& operator<<(std::ostream& out, const connstr& c) {
    out << c.printable();
    return out;
}

} // namespace uh::cluster::db
