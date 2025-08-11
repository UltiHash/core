#include "rr_data_view.h"
#include "impl/address_utils.h"

#include <common/telemetry/log.h>

#include <numeric>
#include <ranges>
#include <unordered_set>

namespace uh::cluster::storage {
rr_data_view::rr_data_view(boost::asio::io_context& ioc, etcd_manager& etcd,
                           std::size_t group_id, group_config group_config,
                           std::size_t service_connections)
    : m_ioc(ioc),
      m_group_config{group_config},
      m_load_balancer{},
      m_storage_index{group_config.storages},
      m_storage_maintainer(
          etcd, ns::root.storage_groups[group_id].storage_hostports,
          service_factory<storage_interface>(ioc, service_connections),
          {m_load_balancer, m_storage_index}) {}

coro<address> rr_data_view::write(std::span<const char> data,
                                  const std::vector<std::size_t>& offsets) {
    const auto [storage_id, client] = m_load_balancer.get();
    auto allocation = co_await client->allocate(data.size());
    address rv = compute_address(offsets, data.size_bytes(), storage_id,
                                 allocation.offset);
    auto refcounts = extract_refcounts(rv);
    co_await client->write(allocation, {data}, refcounts[storage_id]);

    co_return rv;
}
address rr_data_view::compute_address(const std::vector<std::size_t>& offsets,
                                      const std::size_t data_size,
                                      const std::size_t storage_id,
                                      const std::size_t base_offset) const {
    address rv;
    for (auto it = offsets.begin(); it != offsets.end(); it++) {
        auto next = std::next(it);
        std::size_t frag_size =
            next == offsets.end() ? data_size - *it : *next - *it;
        auto frag_offset = pointer_traits::rr::get_global_pointer(
            base_offset + *it, m_group_config.id, storage_id);
        rv.emplace_back(frag_offset, frag_size);
    }
    return rv;
}

coro<shared_buffer<>> rr_data_view::read(const uint128_t& pointer,
                                         size_t size) {

    if (size == 0) {
        throw std::runtime_error("Read size must be larger than zero");
    }

    auto [id, storage_pointer] =
        pointer_traits::rr::get_storage_pointer(pointer);
    auto storage = m_storage_index.at(id);
    co_return co_await storage->read(storage_pointer, size);
}

coro<std::size_t> rr_data_view::read_address(const address& addr,
                                             std::span<char> buffer) {
    co_await perform_for_address<void>(
        m_ioc, addr, pointer_traits::rr::get_storage_pointer,
        [buffer](std::shared_ptr<storage_interface> storage,
                 const storage_address_info& info) -> coro<void> {
            if (storage == nullptr)
                throw std::runtime_error("Storage is not available");
            co_await storage->read_address(info.addr, buffer,
                                           info.pointer_offsets);
        },
        m_storage_index.get());

    co_return addr.data_size();
}

coro<std::size_t> rr_data_view::get_used_space() {
    auto nodes = m_storage_index.get();

    size_t used = 0;
    for (const auto& dn : nodes) {
        if (dn != nullptr) {
            used += co_await dn->get_used_space();
        }
    }
    co_return used;
}

std::vector<std::vector<refcount_t>>
rr_data_view::extract_refcounts(const address& addr) const {
    std::map<uint16_t, std::map<std::size_t, std::size_t>>
        refcount_by_stripe_and_storage;
    size_t stripe_size = m_group_config.get_stripe_size();

    for (const auto& frag : addr.fragments) {
        auto [storage_id, local_pointer] =
            pointer_traits::rr::get_storage_pointer(frag.pointer);
        std::size_t first_stripe = local_pointer / stripe_size;
        std::size_t last_stripe = (local_pointer + frag.size - 1) / stripe_size;
        for (size_t stripe_id = first_stripe; stripe_id <= last_stripe;
             stripe_id++) {
            refcount_by_stripe_and_storage[storage_id][stripe_id]++;
        }
    }

    std::vector<std::vector<refcount_t>> refcounts_by_storage;
    refcounts_by_storage.resize(m_group_config.storages);
    for (const auto& [storage_id, stripe_map] :
         refcount_by_stripe_and_storage) {
        std::vector<refcount_t> refcounts;
        refcounts.reserve(stripe_map.size());
        for (const auto& [stripe_id, count] : stripe_map) {
            refcounts.emplace_back(stripe_id, count);
        }
        refcounts_by_storage[storage_id] = std::move(refcounts);
    }
    return refcounts_by_storage;
}

address rr_data_view::compute_rejected_address(
    const std::vector<std::vector<refcount_t>>& rejected_refcounts,
    const address& original_addr) {
    std::unordered_set<std::size_t> rejected_stripes;
    for (const auto& refcount : rejected_refcounts) {
        for (const auto& rc : refcount) {
            rejected_stripes.insert(rc.stripe_id);
        }
    }

    if (rejected_stripes.empty()) {
        return {};
    }

    address rv;
    for (const auto& frag : original_addr.fragments) {
        auto storage_pointer =
            pointer_traits::rr::get_storage_pointer(frag.pointer).second;
        std::size_t first_stripe =
            storage_pointer / m_group_config.get_stripe_size();
        std::size_t last_stripe = (storage_pointer + frag.size - 1) /
                                  m_group_config.get_stripe_size();
        bool has_overlap = false;
        for (size_t stripe_id = first_stripe; stripe_id <= last_stripe;
             stripe_id++) {
            if (rejected_stripes.contains(stripe_id)) {
                has_overlap = true;
            }
        }
        if (has_overlap) {
            rv.push(frag);
        }
    }

    return rv;
}

[[nodiscard]] coro<address> rr_data_view::link(const address& addr) {
    std::vector<std::vector<refcount_t>> refcounts_by_storage =
        extract_refcounts(addr);

    auto refcounts = co_await run_for_all<std::vector<refcount_t>,
                                          std::shared_ptr<storage_interface>>(
        m_ioc,
        [&](size_t i, auto storage) -> coro<std::vector<refcount_t>> {
            if (storage == nullptr)
                throw std::runtime_error("Storage " + std::to_string(i) +
                                         " is not available");
            co_return co_await storage->link(refcounts_by_storage[i]);
        },
        m_storage_index.get());

    co_return compute_rejected_address(refcounts, addr);
}

coro<std::size_t> rr_data_view::unlink(const address& addr) {
    std::vector<std::vector<refcount_t>> refcounts_by_storage =
        extract_refcounts(addr);

    auto freed_partials =
        co_await run_for_all<std::size_t, std::shared_ptr<storage_interface>>(
            m_ioc,
            [&](size_t i, auto storage) -> coro<std::size_t> {
                if (storage == nullptr)
                    throw std::runtime_error("Storage " + std::to_string(i) +
                                             " is not available");
                co_return co_await storage->unlink(refcounts_by_storage[i]);
            },
            m_storage_index.get());

    std::size_t freed =
        std::accumulate(freed_partials.begin(), freed_partials.end(), 0ull);
    co_return freed;
}

} // namespace uh::cluster::storage
