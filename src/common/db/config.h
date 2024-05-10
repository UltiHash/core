#ifndef CORE_COMMON_DB_CONFIG_H
#define CORE_COMMON_DB_CONFIG_H

#include "pool.h"
#include <string>

namespace uh::cluster::db {

struct config {

    // postgresql connection string
    std::string conn_str = DEFAULT_CONN_STR;

    pool::config directory = {"uh_directory", 2u};

    static constexpr const char* DEFAULT_CONN_STR =
        "host=localhost port=5432 dbname=";
};

} // namespace uh::cluster::db

#endif
