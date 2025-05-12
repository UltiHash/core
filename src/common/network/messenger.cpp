#include "messenger.h"

namespace uh::cluster {

coro<address> messenger::recv_address(const header& message_header) {
    address addr(address::allocated_elements(message_header.size));
    register_read_buffer(addr.pointers);
    register_read_buffer(addr.sizes);
    co_await recv_buffers(message_header);
    co_return std::move(addr);
}

coro<fragment> messenger::recv_fragment(const header& message_header) {
    fragment frag;
    register_read_buffer(frag.pointer.ref_data());
    register_read_buffer(frag.size);
    co_await recv_buffers(message_header);
    co_return frag;
}

coro<allocation_t> messenger::recv_allocation(const header& message_header) {
    allocation_t allocation{};
    register_read_buffer(allocation.offset);
    register_read_buffer(allocation.size);
    co_await recv_buffers(message_header);
    co_return allocation;
}

coro<dedupe_response>
messenger::recv_dedupe_response(const header& message_header) {
    dedupe_response dedupe_resp;
    register_read_buffer(dedupe_resp.effective_size);
    dedupe_resp.addr = address(address::allocated_elements(
        message_header.size - sizeof(dedupe_resp.effective_size)));
    register_read_buffer(dedupe_resp.addr.pointers);
    register_read_buffer(dedupe_resp.addr.sizes);
    co_await recv_buffers(message_header);
    co_return std::move(dedupe_resp);
}

coro<void> messenger::send_write(const write_request& req) {
    auto data = std::get<std::span<const char>>(req.data);
    const size_type data_size = static_cast<size_type>(data.size());
    register_write_buffer(req.allocation.offset);
    register_write_buffer(req.allocation.size);
    register_write_buffer(data_size);
    register_write_buffer(data);
    register_write_buffer(req.offsets);

    co_await send_buffers(STORAGE_WRITE_REQ);
}

coro<write_request> messenger::recv_write(const header& message_header) {
    size_type data_size;
    std::size_t alloc_offset;
    std::size_t alloc_size;
    unique_buffer<char> recv_buffer(message_header.size - sizeof(size_type) -
                                    (2 * sizeof(std::size_t)));
    register_read_buffer(alloc_offset);
    register_read_buffer(alloc_size);
    register_read_buffer(data_size);
    register_read_buffer(recv_buffer);
    co_await recv_buffers(message_header);

    std::size_t offsets_size = recv_buffer.size() - data_size;

    write_request req = {
        .allocation = {.offset = alloc_offset, .size = alloc_size},
        .data = unique_buffer<char>(data_size),
        .offsets =
            std::vector<std::size_t>(offsets_size / sizeof(std::size_t))};
    std::memcpy(std::get<unique_buffer<char>>(req.data).data(),
                recv_buffer.data(), data_size);
    std::memcpy(req.offsets.data(), recv_buffer.data() + data_size,
                offsets_size);
    co_return req;
}

coro<void> messenger::send_address(const message_type type,
                                   const address& addr) {
    register_write_buffer(addr.pointers);
    register_write_buffer(addr.sizes);
    co_await send_buffers(type);
}

coro<void> messenger::send_fragment(const message_type type,
                                    const fragment frag) {
    register_write_buffer(frag.pointer.get_data(), 2);
    register_write_buffer(frag.size);
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
    register_write_buffer(dedupe_resp.addr.pointers);
    register_write_buffer(dedupe_resp.addr.sizes);
    co_await send_buffers(SUCCESS);
}

} // namespace uh::cluster
