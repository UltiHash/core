#pragma once

#include <common/etcd/service_discovery/storage_get_handler.h>

namespace uh::cluster {

struct address_info {
    address addr;
    std::vector<size_t> pointer_offsets;
};

coro<size_t> perform_for_address(
    const address& addr, storage_get_handler& storage_get_handler,
    boost::asio::io_context& ioc,
    std::function<coro<void>(size_t, std::shared_ptr<distributed_storage>,
                             const address_info&)>
        fn,
    const std::vector<size_t>& existing_offsets = {});

} // end namespace uh::cluster
