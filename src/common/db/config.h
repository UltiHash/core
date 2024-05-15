#ifndef CORE_COMMON_DB_CONFIG_H
#define CORE_COMMON_DB_CONFIG_H

#include "pool.h"
#include <CLI/CLI.hpp>
#include <string>

namespace uh::cluster::db {

struct config {

    // host and port of database
    std::string host_port = DEFAULT_HOST_PORT;

    // db user name
    std::string username = DEFAULT_USER;

    // db password
    std::string password = DEFAULT_PASS;

    pool::config directory = {"uh_directory", 2u};

    static constexpr const char* DEFAULT_HOST_PORT = "localhost:5432";
    static constexpr const char* DEFAULT_USER = "";
    static constexpr const char* DEFAULT_PASS = "";
};

void configure(CLI::App& app, config& cfg);

} // namespace uh::cluster::db

#endif
