#pragma once
#include "common/etcd/service_discovery/storage_index.h"
#include "common/service_interfaces/storage_interface.h"

namespace uh::cluster {

struct address_info {
    address addr;
    std::vector<size_t> pointer_offsets;
};

struct node_address_info {
    std::unordered_map<std::shared_ptr<storage_interface>, address_info>
        node_info_map;
    size_t data_size;
};

node_address_info
extract_node_address_map(const address& addr,
                         storage_index& storage_load_balancer,
                         const std::vector<size_t>& existing_offsets = {});

coro<size_t> perform_for_address(
    const address& addr, storage_index& storage_load_balancer,
    boost::asio::io_context& ioc,
    std::function<coro<void>(size_t, std::shared_ptr<storage_interface>,
                             const address_info&)>
        fn,
    const std::vector<size_t>& existing_offsets = {});

} // end namespace uh::cluster
