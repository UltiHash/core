#pragma once

#include <common/types/common_types.h>
#include <entrypoint/directory.h>
#include <storage/global/data_view.h>

namespace uh::cluster::ep {

class garbage_collector {
public:
    static coro<void> collect(
        directory& dir,
        storage::global::global_data_view& gdv);

    static constexpr auto POLL_INTERVAL = std::chrono::seconds(5);
};

} // namespace uh::cluster::ep
