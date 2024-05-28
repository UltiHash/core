#ifndef CORE_COMMON_DB_POOL_H
#define CORE_COMMON_DB_POOL_H

#include "connection.h"
#include "connstr.h"
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <string>

namespace uh::cluster::db {

class pool {
public:
    struct connection_wrapper {
        ~connection_wrapper() { m_pool.put_back(std::move(m_conn)); }

        connection* operator->() { return &m_conn; }

    private:
        friend class pool;
        connection_wrapper(connection&& conn, pool& p)
            : m_conn(std::move(conn)),
              m_pool(p) {}

        connection m_conn;
        pool& m_pool;
    };

    /**
     * Construct a pool for a DBMS reachable under `connstr` and the database
     * `dbname`. Allow for `count` connections.
     */
    pool(connstr conn_str, unsigned count);

    connection_wrapper get();

    void stop();

private:
    friend struct connection_wrapper;

    void put_back(connection&& conn);

    std::list<connection> m_connections;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stop = false;
};

} // namespace uh::cluster::db

#endif
