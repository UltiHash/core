#pragma once

#include <common/etcd/utils.h>
#include <common/types/common_types.h>

namespace uh::cluster {

coro<void> periodic_executor(boost::asio::io_context& io_context,
                             std::chrono::seconds interval,
                             std::function<coro<void>()> task);

coro<void> license_handler(boost::asio::io_context& io_context,
                           etcd_manager& etcd);

} // namespace uh::cluster
