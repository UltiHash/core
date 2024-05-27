#include "db.h"

#include "common/utils/debug.h"

namespace uh::cluster::db {

database::database(const config& cfg)
    : m_cfg(cfg),
      m_directory(connstr(cfg, cfg.directory.dbname), cfg.directory.count) {}

pool::connection_wrapper database::directory() {
    LOG_CONTEXT();

    return m_directory.get();
}

} // namespace uh::cluster::db
