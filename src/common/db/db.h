#ifndef CORE_COMMON_DB_DB_H
#define CORE_COMMON_DB_DB_H

#include "config.h"
#include "pool.h"

namespace uh::cluster::db {

class database {
public:
    database(const config& cfg);

    pool::connection_wrapper directory();

private:
    config m_cfg;
    pool m_directory;
};

} // namespace uh::cluster::db

#endif
