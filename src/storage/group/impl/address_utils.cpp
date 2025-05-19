#include "address_utils.h"

#include "common/coroutines/promise.h"

namespace uh::cluster {

struct node_address_info {
    std::unordered_map<std::size_t, address_info> node_info_map;
    size_t data_size;
};

node_address_info extract_node_address_map(
    const address& addr,
    const std::vector<std::shared_ptr<storage_interface>>& storages,
    std::function<std::pair<std::size_t, uint64_t>(uint128_t)>
        get_storage_pointer) {

    node_address_info info;
    size_t offset = 0;
    for (size_t i = 0; i < addr.size(); ++i) {
        auto frag = addr.get(i);
        const auto [id, storage_ptr] = get_storage_pointer(frag.pointer);
        frag.pointer = storage_ptr;

        auto& node_pos = info.node_info_map[id];
        auto& node_address = node_pos.addr;
        node_address.push(frag);
        node_pos.pointer_offsets.emplace_back(offset);
        offset += frag.size;
    }

    info.data_size = offset;
    return info;
}

coro<size_t> perform_for_address(
    boost::asio::io_context& ioc, const address& addr,
    std::function<std::pair<std::size_t, uint64_t>(uint128_t)>
        get_storage_pointer,
    std::function<coro<void>(size_t, std::shared_ptr<storage_interface>,
                             const address_info&)>
        func,
    const std::vector<std::shared_ptr<storage_interface>>& storages) {

    auto info = extract_node_address_map(addr, storages, get_storage_pointer);

    std::vector<future<void>> futures;
    futures.reserve(info.node_info_map.size());

    auto context = co_await boost::asio::this_coro::context;
    size_t i = 0;
    for (auto& dn : info.node_info_map) {
        promise<void> p;
        futures.emplace_back(p.get_future());
        auto storage = storages[dn.first];
        if (storage == nullptr) {
            throw std::runtime_error("Storage service is not available");
        }
        boost::asio::co_spawn(
            ioc, func(i++, storage, dn.second).continue_trace(context),
            use_promise_cospawn(std::move(p)));
    }

    for (auto& f : futures) {
        co_await f.get();
    }

    co_return info.data_size;
}

} // namespace uh::cluster
