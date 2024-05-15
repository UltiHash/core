#include "config.h"

#include "common/utils/common.h"

namespace uh::cluster::db {

void configure(CLI::App& app, config& cfg) {
    app.add_option("--db-host,-D", cfg.host_port,
                   "PGSQL server address as HOST:PORT")
        ->default_val(cfg.host_port)
        ->envname(ENV_CFG_DB_HOSTPORT);

    app.add_option("--db-user", cfg.username, "PGSQL user name")
        ->default_val(cfg.username)
        ->envname(ENV_CFG_DB_USER);

    app.add_option("--db-pass", cfg.password, "PGSQL password")
        ->default_val(cfg.password)
        ->envname(ENV_CFG_DB_PASS);
}

} // namespace uh::cluster::db
