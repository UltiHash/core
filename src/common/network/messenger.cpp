#include "messenger.h"

#include <ranges>

namespace uh::cluster {

coro<address> messenger::recv_address(const header& message_header) {
    address addr(address::allocated_elements(message_header.size));
    LOG_DEBUG() << "messenge_header.size: "
                << std::to_string(message_header.size);
    register_read_buffer(addr.fragments);
    co_await recv_buffers(message_header);
    co_return addr;
}

coro<fragment> messenger::recv_fragment(const header& message_header) {
    fragment frag;
    register_read_buffer(frag);
    co_await recv_buffers(message_header);
    co_return frag;
}

coro<allocation_t> messenger::recv_allocation(const header& message_header) {
    allocation_t allocation{};
    register_read_buffer(allocation);
    co_await recv_buffers(message_header);
    co_return allocation;
}

coro<std::vector<refcount_t>>
messenger::recv_refcounts(const header& message_header) {
    std::vector<refcount_t> refcounts(message_header.size / sizeof(refcount_t));
    LOG_DEBUG() << "messenge_header.size: "
                << std::to_string(message_header.size);
    register_read_buffer(refcounts);
    co_await recv_buffers(message_header);
    co_return refcounts;
}

coro<dedupe_response>
messenger::recv_dedupe_response(const header& message_header) {
    dedupe_response dedupe_resp;
    register_read_buffer(dedupe_resp.effective_size);
    dedupe_resp.addr = address(address::allocated_elements(
        message_header.size - sizeof(dedupe_resp.effective_size)));
    register_read_buffer(dedupe_resp.addr.fragments);
    co_await recv_buffers(message_header);
    co_return dedupe_resp;
}

coro<void> messenger::send_write(const write_request_view& req) {
    register_write_buffer(req.allocation);

    std::size_t num_refcounts = req.refcounts.size();
    register_write_buffer(num_refcounts);
    std::size_t num_buffers = req.buffers.size();
    register_write_buffer(num_buffers);

    register_write_buffer(req.refcounts);

    auto buffer_sizes_view =
        req.buffers |
        std::views::transform([](const auto& s) { return s.size(); });
    std::vector<std::size_t> buffer_sizes(buffer_sizes_view.begin(),
                                          buffer_sizes_view.end());
    register_write_buffer(buffer_sizes);

    for (const auto& buf : req.buffers) {
        register_write_buffer(buf);
    }

    co_await send_buffers(STORAGE_WRITE_REQ);
}

coro<write_request_store> messenger::recv_write(const header& message_header) {
    allocation_t allocation;
    register_read_buffer(allocation);
    std::size_t num_refcounts;
    register_read_buffer(num_refcounts);
    std::size_t num_buffers;
    register_read_buffer(num_buffers);
    unique_buffer<char> buffer(message_header.size - sizeof(allocation) -
                               sizeof(num_buffers) - sizeof(num_refcounts));
    register_read_buffer(buffer);

    co_await recv_buffers(message_header);

    auto p = buffer.begin();

    std::vector<refcount_t> refcounts = std::vector<refcount_t>(
        reinterpret_cast<refcount_t*>(p),
        reinterpret_cast<refcount_t*>(p) + num_refcounts);

    p += num_refcounts * sizeof(refcount_t);

    auto buffer_sizes =
        std::span<const std::size_t>((std::size_t*)p, num_buffers);

    p += buffer_sizes.size_bytes();

    std::vector<std::span<const char>> buffers;
    buffers.reserve(num_buffers);
    for (std::size_t sz : buffer_sizes) {
        buffers.emplace_back(p, sz);
        p += sz;
    }

    co_return write_request_store{.allocation = std::move(allocation),
                                  .buffers = std::move(buffers),
                                  .refcounts = std::move(refcounts),
                                  .backing_buffer = std::move(buffer)};
}

coro<void> messenger::send_address(const message_type type,
                                   const address& addr) {
    register_write_buffer(addr.fragments);
    co_await send_buffers(type);
}

coro<void> messenger::send_fragment(const message_type type,
                                    const fragment frag) {
    register_write_buffer(frag);
    co_await send_buffers(type);
}

coro<void> messenger::send_allocation(const message_type type,
                                      const allocation_t& allocation) {
    register_write_buffer(allocation.offset);
    register_write_buffer(allocation.size);
    co_await send_buffers(type);
}

coro<void> messenger::send_refcounts(const message_type type,
                                     const std::vector<refcount_t>& refcounts) {
    register_write_buffer(refcounts);
    co_await send_buffers(type);
}

coro<void> messenger::send_dedupe_response(const dedupe_response& dedupe_resp) {
    register_write_buffer(dedupe_resp.effective_size);
    register_write_buffer(dedupe_resp.addr.fragments);
    co_await send_buffers(SUCCESS);
}

} // namespace uh::cluster
