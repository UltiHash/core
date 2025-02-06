#pragma once

namespace uh::cluster::ep::cors {

struct config {
    // how long to keep entries in the cache
    std::chrono::system_clock::duration cache_retention =
        std::chrono::seconds(300);
};

} // namespace uh::cluster::ep::cors
