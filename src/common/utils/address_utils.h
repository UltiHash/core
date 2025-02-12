#pragma once

#include <common/coroutines/promise.h>
#include <common/etcd/service_discovery/storage_get_handler.h>

namespace uh::cluster {

struct address_info {
    address addr;
    std::vector<size_t> pointer_offsets;
};

template <typename service> struct node_address_info {
    std::unordered_map<std::shared_ptr<service>, address_info> node_info_map;
    size_t data_size;
};

template <typename service>
node_address_info<service>
extract_node_address_map(const address& addr,
                         storage_get_handler<service>& storage_get_handler,
                         const std::vector<size_t>& existing_offsets) {

    if (!existing_offsets.empty() and addr.size() != existing_offsets.size()) {
        throw std::invalid_argument(
            "offset size must be equal to the address size");
    }

    node_address_info<service> info;
    size_t offset = 0;
    for (size_t i = 0; i < addr.size(); ++i) {
        const auto frag = addr.get(i);
        auto& node_pos =
            info.node_info_map[storage_get_handler.get(frag.pointer)];
        auto& node_address = node_pos.addr;
        node_address.push(frag);
        if (!existing_offsets.empty()) {
            node_pos.pointer_offsets.emplace_back(existing_offsets.at(i));
        } else {
            node_pos.pointer_offsets.emplace_back(offset);
        }
        offset += frag.size;
    }

    info.data_size = offset;
    return info;
}

template <typename service, typename func>
coro<size_t>
perform_for_address(const address& addr,
                    storage_get_handler<service>& storage_get_handler,
                    boost::asio::io_context& ioc, func fn,
                    const std::vector<size_t>& existing_offsets = {}) {

    auto info =
        extract_node_address_map(addr, storage_get_handler, existing_offsets);

    std::vector<future<void>> futures;
    futures.reserve(info.node_info_map.size());

    size_t i = 0;
    for (auto& dn : info.node_info_map) {
        promise<void> p;
        futures.emplace_back(p.get_future());
        boost::asio::co_spawn(ioc, fn(i++, dn.first, dn.second),
                              use_promise_cospawn(std::move(p)));
    }

    for (auto& f : futures) {
        co_await f.get();
    }

    co_return info.data_size;
}

} // end namespace uh::cluster
