#pragma once

#include <chrono>

namespace uh::cluster::proxy::cache {

struct entry_interface {
    using time_point = std::chrono::system_clock::time_point;

    // TODO: Add corresponding interfaces
    //
    // bool is_fresh();
    // vod revive();
    time_point get_expire_time() const { return expire_time; }

private:
    time_point expire_time;
};

} // namespace uh::cluster::proxy::cache
