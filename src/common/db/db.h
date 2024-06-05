#ifndef CORE_COMMON_DB_DB_H
#define CORE_COMMON_DB_DB_H

#include "common/utils/pool.h"
#include "config.h"
#include "connection.h"
#include <boost/asio.hpp>

namespace uh::cluster::db {

class database {
public:
    database(boost::asio::io_context& ioc, const config& cfg);

    coro<pool<connection>::handle> directory();

private:
    pool<connection> m_directory;
};

} // namespace uh::cluster::db

#endif
