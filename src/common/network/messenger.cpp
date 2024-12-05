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

coro<uint128_t> messenger::recv_uint128_t(const header& message_header) {
    uint128_t num;
    register_read_buffer(num.ref_data());
    co_await recv_buffers(message_header);
    co_return num;
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

coro<void> messenger::send_ds_write(context& ctx, const ds_write_request& req) {
    register_write_buffer(req.ds_id);
    register_write_buffer(req.pointer);
    register_write_buffer(std::get<std::string_view>(req.data));
    co_await send_buffers(ctx, STORAGE_DS_WRITE_REQ);
}

coro<ds_write_request> messenger::recv_ds_write(const header& message_header) {
    ds_write_request req{
        .data = unique_buffer<>(message_header.size -
                                sizeof(ds_write_request::ds_id) -
                                sizeof(ds_write_request::pointer))};
    register_read_buffer(req.ds_id);
    register_read_buffer(req.pointer);
    const auto& buf = std::get<unique_buffer<>>(req.data);
    register_read_buffer(buf.data(), buf.size());
    co_await recv_buffers(message_header);
    co_return req;
}

coro<void> messenger::send_ds_read(context& ctx, const ds_read_request& req) {
    register_write_buffer(req.ds_id);
    register_write_buffer(req.pointer);
    register_write_buffer(req.size);
    co_await send_buffers(ctx, STORAGE_DS_READ_REQ);
}

coro<ds_read_request> messenger::recv_ds_read(const header& message_header) {
    ds_read_request req;
    register_read_buffer(req.ds_id);
    register_read_buffer(req.pointer);
    register_read_buffer(req.size);
    co_await recv_buffers(message_header);
    co_return req;
}

coro<void> messenger::send_address(context& ctx, const message_type type,
                                   const address& addr) {
    register_write_buffer(addr.pointers);
    register_write_buffer(addr.sizes);
    co_await send_buffers(ctx, type);
}

coro<void> messenger::send_fragment(context& ctx, const message_type type,
                                    const fragment frag) {
    register_write_buffer(frag.pointer.get_data(), 2);
    register_write_buffer(frag.size);
    co_await send_buffers(ctx, type);
}

coro<void> messenger::send_uint128_t(context& ctx, const message_type type,
                                     const uint128_t num) {
    register_write_buffer(num.get_data(), 2);
    co_await send_buffers(ctx, type);
}

coro<void> messenger::send_dedupe_response(context& ctx,
                                           const dedupe_response& dedupe_resp) {
    register_write_buffer(dedupe_resp.effective_size);
    register_write_buffer(dedupe_resp.addr.pointers);
    register_write_buffer(dedupe_resp.addr.sizes);
    co_await send_buffers(ctx, SUCCESS);
}

} // namespace uh::cluster
