#pragma once

#include <boost/asio.hpp>
#include <common/service_interfaces/storage_interface.h>
#include <common/types/common_types.h>
#include <entrypoint/directory.h>

namespace uh::cluster::ep {

class garbage_collector {
public:
    garbage_collector(boost::asio::io_context& ctx, directory& dir,
                      storage_interface& gdv);

private:
    static constexpr auto POLL_INTERVALL = std::chrono::seconds(5);
    static constexpr const char* EP_GC_INITIAL_CONTEXT_NAME =
        "ep-garbe-collector";
    coro<void> collect();

    directory& m_dir;
    storage_interface& m_gdv;
};

} // namespace uh::cluster::ep
