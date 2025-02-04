#include "usage.h"
#include <format>

namespace uh::cluster {

coro<std::size_t>
usage::get_usage_for_interval(const utc_time& interval_infimum,
                              const utc_time& interval_supremum) {
    auto handle = co_await m_db.get();

    auto row =
        co_await handle->execb("select uh_compute_usage($1, $2)",
                               std::format("{0:%F %T}", interval_infimum),
                               std::format("{0:%F %T}", interval_supremum));

    if (!row) {
        co_return 0;
    }

    co_return *row->number(0);
}

} // namespace uh::cluster
