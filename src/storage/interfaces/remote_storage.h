#pragma once

#include "distributed.h"
#include <common/network/client.h>

namespace uh::cluster {

struct remote_storage : public distributed_storage {

    explicit remote_storage(client storage_service)
        : m_storage_service(std::move(storage_service)) {}

    coro<address> write(context& ctx, std::span<const char> data,
                        const std::vector<std::size_t>& offsets) override {
        auto m = co_await m_storage_service.acquire_messenger();
        LOG_DEBUG() << ctx.peer() << ": sending STORAGE_WRITE_REQ ["
                    << m->local() << " -> " << m->peer() << "]";
        write_request req = {.offsets = offsets, .data = data};

        co_await m->send_write(ctx, req);
        const auto message_header = co_await m->recv_header();
        co_return co_await m->recv_address(message_header);
    }

    coro<std::size_t> read(context& ctx, const address& addr,
                           std::span<char> buffer) override {
        auto m = co_await m_storage_service.acquire_messenger();

        co_await m->send_address(ctx, STORAGE_READ_ADDRESS_REQ, addr);
        const auto h = co_await m->recv_header();

        m->register_read_buffer(buffer);
        co_return co_await m->recv_buffers(h);
    }

    coro<void> read_address(context& ctx, const address& addr,
                            std::span<char> buffer,
                            const std::vector<size_t>& offsets) override {
        auto m = co_await m_storage_service.acquire_messenger();

        co_await m->send_address(ctx, STORAGE_READ_ADDRESS_REQ, addr);
        const auto h = co_await m->recv_header();

        m->reserve_read_buffers(addr.size());
        for (size_t i = 0; i < addr.size(); ++i) {
            m->register_read_buffer(buffer.data() + offsets.at(i),
                                    addr.sizes[i]);
        }

        co_await m->recv_buffers(h);
    }

    coro<address> link(context& ctx, const address& addr) override {
        auto m = co_await m_storage_service.acquire_messenger();
        LOG_DEBUG() << ctx.peer() << ": sending STORAGE_LINK_REQ ["
                    << m->local() << " -> " << m->peer() << "]";
        co_await m->send_address(ctx, STORAGE_LINK_REQ, addr);
        const auto message_header = co_await m->recv_header();
        co_return co_await m->recv_address(message_header);
    }

    coro<std::size_t> unlink(context& ctx, const address& addr) override {
        auto m = co_await m_storage_service.acquire_messenger();
        LOG_DEBUG() << ctx.peer() << ": sending STORAGE_UNLINK_REQ ["
                    << m->local() << " -> " << m->peer() << "]";
        co_await m->send_address(ctx, STORAGE_UNLINK_REQ, addr);
        const auto message_header = co_await m->recv_header();
        co_return co_await m->recv_primitive<size_t>(message_header);
    }

    coro<std::size_t> get_used_space(context& ctx) override {
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
                        std::span<const char> data) override {
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

class remote_factory {
public:
    remote_factory(boost::asio::io_context& ioc, std::size_t connections);

    std::shared_ptr<remote_storage> make_service(const std::string& hostname,
                                                 uint16_t port, int);

private:
    boost::asio::io_context& m_ioc;
    std::size_t m_connections;
};

} // namespace uh::cluster
