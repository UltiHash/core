#pragma once

#include <boost/asio.hpp>
#include <common/types/common_types.h>
#include <entrypoint/directory.h>
#include <storage/global/data_view.h>

#include <condition_variable>
#include <mutex>
#include <stop_token>

namespace uh::cluster::ep {

class garbage_collector {
public:
    garbage_collector(boost::asio::io_context& ctx, directory& dir,
                      storage::global::global_data_view& gdv);

    void stop();

private:
    static constexpr auto POLL_INTERVALL = std::chrono::seconds(5);
    static constexpr const char* EP_GC_INITIAL_CONTEXT_NAME =
        "ep-garbe-collector";
    coro<void> collect();

    std::stop_source m_stop;
    directory& m_dir;
    storage::global::global_data_view& m_gdv;

    bool m_stopped = false;
    std::mutex m_mutex;
    std::condition_variable m_condition;
};

} // namespace uh::cluster::ep
