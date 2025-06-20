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

coro<void> messenger::send_write(const write_request& req) {
    register_write_buffer(req.allocation);

    std::size_t num_offsets = req.offsets.size();
    register_write_buffer(num_offsets);
    std::size_t num_buffers = req.buffers.size();
    register_write_buffer(num_buffers);

    register_write_buffer(req.offsets);

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

coro<std::pair<write_request, unique_buffer<char>>>
messenger::recv_write(const header& message_header) {
    allocation_t allocation;
    register_read_buffer(allocation);
    std::size_t num_offsets;
    register_read_buffer(num_offsets);
    std::size_t num_buffers;
    register_read_buffer(num_buffers);
    unique_buffer<char> buffer(message_header.size - sizeof(allocation) -
                               sizeof(num_buffers) - sizeof(num_offsets));
    register_read_buffer(buffer);

    co_await recv_buffers(message_header);

    auto p = buffer.begin();

    auto offsets = std::span<const std::size_t>((std::size_t*)p, num_offsets);

    p += offsets.size_bytes();

    auto buffer_sizes =
        std::span<const std::size_t>((std::size_t*)p, num_buffers);

    p += buffer_sizes.size_bytes();

    std::vector<std::span<const char>> buffers;
    buffers.reserve(num_buffers);
    for (std::size_t sz : buffer_sizes) {
        buffers.emplace_back(p, sz);
        p += sz;
    }

    write_request req = {.allocation = std::move(allocation),
                         .buffers = std::move(buffers),
                         .offsets = std::move(offsets)};

    co_return std::make_pair(std::move(req), std::move(buffer));
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

coro<void> messenger::send_dedupe_response(const dedupe_response& dedupe_resp) {
    register_write_buffer(dedupe_resp.effective_size);
    register_write_buffer(dedupe_resp.addr.fragments);
    co_await send_buffers(SUCCESS);
}

} // namespace uh::cluster
