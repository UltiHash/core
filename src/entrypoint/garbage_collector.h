#pragma once

#include <boost/asio.hpp>
#include <common/coroutines/coro_util.h>
#include <common/types/common_types.h>
#include <entrypoint/directory.h>
#include <storage/global/data_view.h>

namespace uh::cluster::ep {

class garbage_collector {
public:
    garbage_collector(boost::asio::io_context& ctx, directory& dir,
                      storage::global::global_data_view& gdv);

private:
    static constexpr auto POLL_INTERVALL = std::chrono::seconds(5);
    static constexpr const char* EP_GC_INITIAL_CONTEXT_NAME =
        "ep-garbe-collector";
    coro<void> collect();

    directory& m_dir;
    storage::global::global_data_view& m_gdv;
    coro_task m_task;
};

} // namespace uh::cluster::ep
