
#ifndef UH_CLUSTER_REMOTE_STORAGE_H
#define UH_CLUSTER_REMOTE_STORAGE_H

#include "common/network/client.h"
#include "common/service_interfaces/storage_interface.h"

namespace uh::cluster {

struct remote_storage : public storage_interface {

    explicit remote_storage(client storage_service)
        : m_storage_service(std::move(storage_service)) {}

    coro<address> write(context& ctx, const std::string_view& data) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send(ctx, STORAGE_WRITE_REQ, data);
        const auto message_header = co_await m->recv_header();
        co_return co_await m->recv_address(message_header);
    }

    coro<void> read_fragment(context& ctx, char* buffer,
                             const fragment& frag) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send_fragment(ctx, STORAGE_READ_FRAGMENT_REQ, frag);
        const auto h = co_await m->recv_header();
        if (h.size != frag.size) [[unlikely]] {
            throw std::runtime_error("Incomplete fragment");
        }
        m->register_read_buffer(buffer, frag.size);
        co_await m->recv_buffers(h);
    }

    coro<shared_buffer<>> read(context& ctx, const uint128_t& pointer,
                               size_t size) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send_fragment(ctx, STORAGE_READ_REQ, {pointer, size});
        const auto h = co_await m->recv_header();
        shared_buffer<> buffer(h.size);
        m->register_read_buffer(buffer.data(), buffer.size());
        co_await m->recv_buffers(h);
        co_return buffer;
    }

    coro<void> read_address(context& ctx, char* buffer, const address& addr,

                            const std::vector<size_t>& offsets) override {
        auto m = co_await m_storage_service.acquire_messenger();

        co_await m->send_address(ctx, STORAGE_READ_ADDRESS_REQ, addr);
        const auto h = co_await m->recv_header();

        m->reserve_read_buffers(addr.size());
        for (size_t i = 0; i < addr.size(); ++i) {
            m->register_read_buffer(buffer + offsets.at(i), addr.sizes[i]);
        }

        co_await m->recv_buffers(h);
    }

    coro<address> link(context& ctx, const address& addr) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send_address(ctx, STORAGE_LINK_REQ, addr);
        const auto message_header = co_await m->recv_header();
        co_return co_await m->recv_address(message_header);
    }

    coro<void> unlink(context& ctx, const address& addr) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send_address(ctx, STORAGE_UNLINK_REQ, addr);
        co_await m->recv_header();
    }

    coro<size_t> get_used_space(context& ctx) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send(ctx, STORAGE_USED_REQ, {});
        const auto message_header = co_await m->recv_header();
        co_return co_await m->recv_primitive<size_t>(message_header);
    }

    coro<std::map<size_t, size_t>> get_ds_size_map(context& ctx) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send(ctx, STORAGE_DS_INFO_REQ, {});
        const auto message_header = co_await m->recv_header();
        co_return co_await m->recv_map<size_t, size_t>(ctx, message_header);
    }

    coro<void> ds_write(context& ctx, uint32_t ds_id, uint64_t pointer,
                        const std::string_view& data) override {
        auto m = co_await m_storage_service.acquire_messenger();
        ds_write_request req{.ds_id = ds_id, .pointer = pointer, .data = data};
        co_await m->send_ds_write(ctx, req);
        co_await m->recv_header();
    }

    coro<void> ds_read(context& ctx, uint32_t ds_id, uint64_t pointer,
                       size_t size, char* buffer) override {
        auto m = co_await m_storage_service.acquire_messenger();
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

private:
    client m_storage_service;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_REMOTE_STORAGE_H
