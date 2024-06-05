#include "db.h"

namespace uh::cluster::db {

namespace {

auto make_factory(const config& cfg, const config::database& db_cfg) {

    connstr cs(cfg, db_cfg.dbname);

    return [cs]() {
        LOG_INFO() << "connecting to " << cs;

        auto conn = std::make_unique<connection>(cs);
        auto res = conn->exec("SELECT version();");
        LOG_INFO() << "connected to `" << cs << "`: " << *res.string(0, 0);
        return conn;
    };
}

} // namespace

database::database(boost::asio::io_context& ioc, const config& cfg)
    : m_directory(ioc, make_factory(cfg, cfg.directory), cfg.directory.count) {}

coro<pool<connection>::handle> database::directory() {
    return m_directory.get();
}

} // namespace uh::cluster::db
