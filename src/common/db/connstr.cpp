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
