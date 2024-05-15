#include "db.h"

#include <stdexcept>

namespace uh::cluster::db {

namespace {

std::string connection_string(const config& cfg) {

    std::string rv = "postgresql://";

    if (!cfg.username.empty()) {
        rv += cfg.username;

        if (!cfg.password.empty()) {
            rv += ":" + cfg.password;
        }

        rv += "@";
    }

    rv += cfg.host_port + "/";

    return rv;
}

} // namespace

database::database(const config& cfg)
    : m_cfg(cfg),
      m_directory(connection_string(cfg), cfg.directory) {}

pool::connection_wrapper database::directory() { return m_directory.get(); }

} // namespace uh::cluster::db
