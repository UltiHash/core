#include "protocol.h"

#include <common/telemetry/log.h>

namespace uh::cluster::sn {

coro<address> write(client::acquired_messenger& m, context& ctx,
                    std::span<const char> data,
                    const std::vector<std::size_t>& offsets) {

    LOG_DEBUG() << ctx.peer() << ": sending STORAGE_WRITE_REQ [" << m->local()
                << " -> " << m->peer() << "]";

    write_request req = {.offsets = offsets, .data = data};
    co_await m->send_write(ctx, req);

    auto message_header = co_await m->recv_header();
    co_return co_await m->recv_address(message_header);
}

coro<std::size_t> read(client::acquired_messenger& m, context& ctx,
                       const address& addr, std::span<char> buffer) {

    co_await m->send_address(ctx, STORAGE_READ_ADDRESS_REQ, addr);

    const auto h = co_await m->recv_header();
    m->register_read_buffer(buffer);
    co_return co_await m->recv_buffers(h);
}

coro<std::size_t> read_address(client::acquired_messenger& m, context& ctx,
                        const address& addr, std::span<char> buffer,
                        const std::vector<size_t>& offsets) {

    co_await m->send_address(ctx, STORAGE_READ_ADDRESS_REQ, addr);

    auto h = co_await m->recv_header();
    m->reserve_read_buffers(addr.size());
    for (size_t i = 0; i < addr.size(); ++i) {
        m->register_read_buffer(buffer.data() + offsets.at(i), addr.sizes[i]);
    }

    co_return co_await m->recv_buffers(h);
}

coro<address> link(client::acquired_messenger& m, context& ctx,
                   const address& addr) {

    LOG_DEBUG() << ctx.peer() << ": sending STORAGE_LINK_REQ [" << m->local()
                << " -> " << m->peer() << "]";

    co_await m->send_address(ctx, STORAGE_LINK_REQ, addr);

    auto h = co_await m->recv_header();
    co_return co_await m->recv_address(h);
}

coro<std::size_t> unlink(client::acquired_messenger& m, context& ctx,
                         const address& addr) {

    LOG_DEBUG() << ctx.peer() << ": sending STORAGE_UNLINK_REQ [" << m->local()
                << " -> " << m->peer() << "]";

    co_await m->send_address(ctx, STORAGE_UNLINK_REQ, addr);
    const auto message_header = co_await m->recv_header();
    co_return co_await m->recv_primitive<size_t>(message_header);
}

coro<std::size_t> get_used_space(client::acquired_messenger& m, context& ctx) {
    co_await m->send(ctx, STORAGE_USED_REQ, {});
    const auto message_header = co_await m->recv_header();
    co_return co_await m->recv_primitive<size_t>(message_header);
}

coro<std::map<size_t, size_t>> get_ds_size_map(client::acquired_messenger& m,
                                               context& ctx) {
    co_await m->send(ctx, STORAGE_DS_INFO_REQ, {});
    const auto message_header = co_await m->recv_header();
    co_return co_await m->recv_map<size_t, size_t>(ctx, message_header);
}

coro<void> ds_write(client::acquired_messenger& m, context& ctx, uint32_t ds_id,
                    uint64_t pointer, std::span<const char> data) {
    ds_write_request req{.ds_id = ds_id, .pointer = pointer, .data = data};
    co_await m->send_ds_write(ctx, req);
    co_await m->recv_header();
}

coro<void> ds_read(client::acquired_messenger& m, context& ctx, uint32_t ds_id,
                   uint64_t pointer, size_t size, char* buffer) {
    co_await m->send_ds_read(
        ctx, {.ds_id = ds_id, .pointer = pointer, .size = size});
    const auto h = co_await m->recv_header();
    if (h.size != size) {
        throw std::runtime_error(
            "mistmatched read size with requested size in ds_read");
    }
    m->register_read_buffer(buffer, size);
    co_await m->recv_buffers(h);
}

} // namespace uh::cluster::sn
