#pragma once

#include <boost/asio.hpp>
#include <common/types/common_types.h>
#include <entrypoint/directory.h>
#include <storage/interface.h>

namespace uh::cluster::ep {

class garbage_collector {
public:
    garbage_collector(boost::asio::io_context& ctx, directory& dir,
                      sn::interface& gdv);

private:
    static constexpr auto POLL_INTERVALL = std::chrono::seconds(5);
    static constexpr const char* EP_GC_INITIAL_CONTEXT_NAME =
        "ep-garbe-collector";
    coro<void> collect();

    directory& m_dir;
    sn::interface& m_gdv;
};

} // namespace uh::cluster::ep
