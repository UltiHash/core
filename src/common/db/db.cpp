#include "db.h"

#include <stdexcept>

namespace uh::cluster::db {

database::database(const config& cfg)
    : m_cfg(cfg),
      m_directory(cfg.conn_str, cfg.directory) {}

pool::connection_wrapper database::directory() { return m_directory.get(); }

} // namespace uh::cluster::db
