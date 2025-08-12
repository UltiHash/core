#pragma once
#include <common/coroutines/coro_util.h>
#include <common/etcd/service_discovery/storage_index.h>
#include <common/service_interfaces/storage_interface.h>

namespace uh::cluster {

struct storage_address_info {
    storage_address addr;
    std::vector<size_t> pointer_offsets;
};

std::unordered_map<std::size_t, storage_address_info> extract_node_address_map(
    const address& addr,
    std::function<std::pair<std::size_t, uint64_t>(uint128_t)>
        get_storage_pointer);

template <typename R>
coro<std::conditional_t<std::is_void_v<R>, void,
                        std::unordered_map<std::size_t, R>>>
perform_for_address(
    boost::asio::io_context& ioc, const address& addr,
    std::function<std::pair<std::size_t, uint64_t>(uint128_t)>
        get_storage_pointer,
    std::function<coro<R>(std::shared_ptr<storage_interface>,
                          const storage_address_info&)>
        func,
    const std::vector<std::shared_ptr<storage_interface>>& storages) {

    auto addr_map = extract_node_address_map(addr, get_storage_pointer);

    co_return co_await run_for_all<R, std::size_t, storage_address_info>(
        ioc,
        [&storages, &func](std::size_t id,
                           const storage_address_info& addr_info) -> coro<R> {
            co_return co_await func(storages[id], addr_info);
        },
        addr_map);
}

} // end namespace uh::cluster
