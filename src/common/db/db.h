#ifndef CORE_COMMON_DB_DB_H
#define CORE_COMMON_DB_DB_H

#include "common/utils/pool.h"
#include "config.h"
#include "connection.h"

namespace uh::cluster::db {

/**
 * Create a connection factory that can be passed to `uh::cluster::pool`.
 */
inline auto connection_factory(boost::asio::io_context& ioc, const config& cfg,
                               const config::database& db_cfg) {

    connstr cs(cfg, db_cfg.dbname);

    return [cs, &ioc]() {
        LOG_INFO() << "connecting to " << cs;

        auto conn = std::make_unique<connection>(ioc, cs);
        auto row = conn->raw_exec("SELECT version();");
        LOG_INFO() << "connected to `" << cs << "`: " << *row->string(0);
        return conn;
    };
}

} // namespace uh::cluster::db

#endif
