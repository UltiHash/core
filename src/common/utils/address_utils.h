#ifndef ADDRESS_UTILS_H
#define ADDRESS_UTILS_H
#include "common/etcd/service_discovery/storage_get_handler.h"
#include "common/service_interfaces/storage_interface.h"

namespace uh::cluster {

struct address_node_info {
    std::vector<std::shared_ptr<storage_interface>> nodes;
    std::unordered_map<std::shared_ptr<storage_interface>, address>
        node_address_map;
    std::unordered_map<std::shared_ptr<storage_interface>, std::vector<size_t>>
        node_data_offsets_map;
    size_t data_size;
};

address_node_info
extract_node_address_map(const address& addr,
                         storage_get_handler& storage_get_handler,
                         const std::vector<size_t>& existing_offsets = {});

coro<void> perform_for_address(
    const address& addr, storage_get_handler& storage_get_handler,
    boost::asio::io_context& ioc,
    std::function<coro<void>(size_t, std::shared_ptr<storage_interface>,
                             const address&)>
        fn);

} // end namespace uh::cluster

#endif // ADDRESS_UTILS_H
