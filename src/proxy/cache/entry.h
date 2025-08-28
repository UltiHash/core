#pragma once

#include <chrono>

namespace uh::cluster::proxy::cache {

template <typename Derived> struct entry_interface {
    using time_point = std::chrono::system_clock::time_point;

    // TODO: Make expiration duration configurable
    entry_interface()
        : expire_time{std::chrono::system_clock::now() +
                      std::chrono::seconds(10)} {
        static_assert(
            requires(Derived d) { d.size(); }, "Derived must implement size()");
    }

    // TODO: Add corresponding interfaces
    //
    // bool is_fresh();
    // vod revive();
    time_point get_expire_time() const { return expire_time; }

private:
    time_point expire_time;
};

} // namespace uh::cluster::proxy::cache
