#ifndef COMMON_DB_CONNSTR_H
#define COMMON_DB_CONNSTR_H

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

#endif
