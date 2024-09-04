#include "address_utils.h"

#include "common/coroutines/awaitable_promise.h"

namespace uh::cluster {

address_node_info
extract_node_address_map(const address& addr,
                         storage_get_handler& storage_get_handler,
                         const std::vector<size_t>& existing_offsets) {
    address_node_info info;
    size_t offset = 0;
    for (size_t i = 0; i < addr.size(); ++i) {
        const auto frag = addr.get(i);
        auto n = storage_get_handler.get(frag.pointer);
        auto& node_address = info.node_address_map[n];
        if (node_address.empty()) {
            info.nodes.emplace_back(n);
        }
        node_address.push(frag);
        if (!existing_offsets.empty()) {
            info.node_data_offsets_map[n].emplace_back(existing_offsets.at(i));
        } else {
            info.node_data_offsets_map[n].emplace_back(offset);
        }
        offset += frag.size;
    }

    info.data_size = offset;
    return info;
}

coro<void> perform_for_address(
    const address& addr, storage_get_handler& storage_get_handler,
    boost::asio::io_context& ioc,
    std::function<coro<void>(size_t, std::shared_ptr<storage_interface>,
                             const address&)>
        fn) {

    auto info = extract_node_address_map(addr, storage_get_handler);

    std::vector<std::shared_ptr<awaitable_promise<void>>> proms;
    proms.reserve(info.nodes.size());

    size_t i = 0;
    for (auto& dn : info.nodes) {
        proms.emplace_back(std::make_shared<awaitable_promise<void>>(ioc));
        boost::asio::co_spawn(ioc, fn(i++, dn, info.node_address_map[dn]),
                              use_awaitable_promise_cospawn(proms.back()));
    }

    for (auto& p : proms) {
        co_await p->get();
    }
}

} // namespace uh::cluster