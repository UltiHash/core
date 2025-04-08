#include "address_utils.h"

#include "common/coroutines/promise.h"

namespace uh::cluster {

node_address_info
extract_node_address_map(const address& addr,
                         storage_index& storage_load_balancer,
                         const std::vector<size_t>& existing_offsets) {

    if (!existing_offsets.empty() and addr.size() != existing_offsets.size()) {
        throw std::invalid_argument(
            "offset size must be equal to the address size");
    }

    node_address_info info;
    size_t offset = 0;
    for (size_t i = 0; i < addr.size(); ++i) {
        const auto frag = addr.get(i);
        auto& node_pos =
            info.node_info_map[storage_load_balancer.get(frag.pointer)];
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

coro<size_t> perform_for_address(
    const address& addr, storage_index& storage_load_balancer,
    boost::asio::io_context& ioc,
    std::function<coro<void>(size_t, std::shared_ptr<storage_interface>,
                             const address_info&)>
        fn,
    const std::vector<size_t>& existing_offsets) {

    auto info =
        extract_node_address_map(addr, storage_load_balancer, existing_offsets);

    std::vector<future<void>> futures;
    futures.reserve(info.node_info_map.size());

    auto context = co_await boost::asio::this_coro::context;
    size_t i = 0;
    for (auto& dn : info.node_info_map) {
        promise<void> p;
        futures.emplace_back(p.get_future());

        boost::asio::co_spawn(
            ioc, fn(i++, dn.first, dn.second).continue_trace(context),
            use_promise_cospawn(std::move(p)));
    }

    for (auto& f : futures) {
        co_await f.get();
    }

    co_return info.data_size;
}

} // namespace uh::cluster
