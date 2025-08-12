#pragma once

#include <common/db/db.h>
#include <common/types/common_types.h>

namespace uh::cluster {

class usage {
public:
    usage(boost::asio::io_context& ioc, const db::config& cfg)
        : m_db(connection_factory(ioc, cfg, cfg.directory),
               cfg.directory.count) {}

    coro<std::size_t> get_usage_for_interval(const utc_time& interval_infimum,
                                             const utc_time& interval_supremum);

private:
    pool<db::connection> m_db;
};
} // namespace uh::cluster
