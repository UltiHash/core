#include "pool.h"

#include "common/telemetry/log.h"

namespace uh::cluster::db {

pool::pool(connstr conn_str, unsigned count) {
    if (count < 1) {
        throw std::runtime_error("at least one connection is required");
    }

    LOG_INFO() << "connecting to " << conn_str;
    for (auto n = 0u; n < count; ++n) {
        m_connections.emplace_back(conn_str);
    }

    auto& conn = m_connections.front();
    auto res = conn.exec("SELECT version();");
    LOG_INFO() << "connected to `" << conn_str
               << "`: " << PQgetvalue(res, 0, 0);
}

void pool::stop() {
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_stop = true;
    }

    m_cv.notify_all();
}

pool::connection_wrapper pool::get() {
    std::unique_lock<std::mutex> lk(m_mutex);

    m_cv.wait(lk, [this]() { return !m_connections.empty() || m_stop; });

    if (m_stop) {
        throw std::runtime_error("operation cancelled");
    }

    auto conn = std::move(m_connections.front());
    m_connections.pop_front();
    return connection_wrapper(std::move(conn), *this);
}

void pool::put_back(connection&& conn) {
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_connections.emplace_back(std::move(conn));
    }

    m_cv.notify_one();
}

} // namespace uh::cluster::db
