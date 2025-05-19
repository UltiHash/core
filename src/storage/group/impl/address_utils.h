#pragma once
#include "common/etcd/service_discovery/storage_index.h"
#include "common/service_interfaces/storage_interface.h"

namespace uh::cluster {

struct address_info {
    address addr;
    std::vector<size_t> pointer_offsets;
};

coro<size_t> perform_for_address(
    const address& addr, storage_index& storages, boost::asio::io_context& ioc,
    std::function<coro<void>(size_t, std::shared_ptr<storage_interface>,
                             const address_info&)>
        fn,
    std::function<std::pair<std::size_t, uint64_t>(uint128_t)>
        get_storage_pointer);

} // end namespace uh::cluster
