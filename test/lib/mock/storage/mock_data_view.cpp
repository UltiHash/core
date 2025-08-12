#include "mock_data_view.h"

#include <storage/group/impl/address_utils.h>

namespace uh::cluster {
mock_data_view::mock_data_view(mock_data_store& storage)
    : m_storage{storage} {}

address compute_address(const std::vector<std::size_t>& offsets,
                        const std::size_t data_size,
                        const allocation_t& allocation) {
    address rv;
    std::size_t base_offset = allocation.offset;
    for (auto it = offsets.begin(); it != offsets.end(); it++) {
        auto next = std::next(it);
        std::size_t frag_size =
            next == offsets.end() ? data_size - *it : *next - *it;
        rv.emplace_back(base_offset + *it, frag_size);
    }
    return rv;
}

std::vector<refcount_t> extract_refcounts(const address& addr) {
    std::map<std::size_t, std::size_t> refcount_by_stripe;

    for (const auto& frag : addr.fragments) {
        auto group_pointer = pointer_traits::get_group_pointer(frag.pointer);
        std::size_t first_stripe = group_pointer / DEFAULT_PAGE_SIZE;
        std::size_t last_stripe =
            (group_pointer + frag.size - 1) / DEFAULT_PAGE_SIZE;
        for (size_t stripe_id = first_stripe; stripe_id <= last_stripe;
             stripe_id++) {
            refcount_by_stripe[stripe_id]++;
        }
    }

    std::vector<refcount_t> refcounts;
    refcounts.reserve(refcount_by_stripe.size());
    for (const auto& [stripe_id, count] : refcount_by_stripe) {
        refcounts.emplace_back(stripe_id, count);
    }
    return refcounts;
}

coro<address> mock_data_view::write(std::span<const char> data,
                                    const std::vector<std::size_t>& offsets) {
    auto alloc = m_storage.allocate(data.size());
    auto addr = compute_address(offsets, data.size(), alloc);
    auto refcounts = extract_refcounts(addr);
    m_storage.write(alloc, std::vector<std::span<const char>>{data}, refcounts);
    co_return addr;
}

coro<shared_buffer<>> mock_data_view::read(const uint128_t& pointer,
                                           size_t size) {
    shared_buffer<char> buffer(size);
    m_storage.read(pointer, buffer.span());
    co_return buffer;
}

coro<std::size_t> mock_data_view::read_address(const address& addr,
                                               std::span<char> buffer) {
    auto size = 0u;
    for (size_t i = 0; i < addr.size(); ++i) {
        auto frag = addr.get(i);
        m_storage.read(frag.pointer, buffer.first(frag.size));
        buffer = buffer.subspan(frag.size);
        size += frag.size;
    }

    co_return size;
}

coro<std::size_t> mock_data_view::get_used_space() {
    co_return m_storage.get_used_space();
}

[[nodiscard]] coro<address> mock_data_view::link(const address& addr) {
    auto refcounts = extract_refcounts(addr);
    m_storage.link(refcounts);

    address rv;
    co_return rv; // This is a placeholder, actual implementation should return
                  // a valid address
}

coro<std::size_t> mock_data_view::unlink(const address& addr) {
    auto refcounts = extract_refcounts(addr);
    co_return m_storage.unlink(refcounts);
}

} // namespace uh::cluster
