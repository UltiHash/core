#ifndef CORE_ENTRYPOINT_USER_DB_BACKEND_H
#define CORE_ENTRYPOINT_USER_DB_BACKEND_H

#include "backend.h"
#include "common/db/connection.h"
#include "common/utils/pool.h"
#include "entrypoint/policy/parser.h"

namespace uh::cluster::ep::user {

class db_backend : public backend {
public:
    db_backend(boost::asio::io_context& ioc, const db::config& cfg);

    coro<user> find(std::string_view) override;

private:
    pool<db::connection> m_db;
};

} // namespace uh::cluster::ep::user

#endif
