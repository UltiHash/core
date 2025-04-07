#pragma once

#include <common/network/client.h>
#include <common/service_interfaces/service_factory.h>
#include <common/service_interfaces/storage_interface.h>

namespace uh::cluster {

struct remote_storage : public storage_interface {

    explicit remote_storage(client storage_service)
        : m_storage_service(std::move(storage_service)) {}

    coro<address> write(std::span<const char> data,
                        const std::vector<std::size_t>& offsets) override {
        auto m = co_await m_storage_service.acquire_messenger();
        write_request req = {.offsets = offsets, .data = data};

        co_await m->send_write(req);
        const auto message_header = co_await m->recv_header();
        co_return co_await m->recv_address(message_header);
    }

    coro<void> read_fragment(char* buffer, const fragment& frag) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send_fragment(STORAGE_READ_FRAGMENT_REQ, frag);
        const auto h = co_await m->recv_header();
        if (h.size != frag.size) [[unlikely]] {
            throw std::runtime_error("Incomplete fragment");
        }
        m->register_read_buffer(buffer, frag.size);
        co_await m->recv_buffers(h);
    }

    coro<shared_buffer<>> read(const uint128_t& pointer, size_t size) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send_fragment(STORAGE_READ_REQ, {pointer, size});
        const auto h = co_await m->recv_header();
        shared_buffer<> buffer(h.size);
        m->register_read_buffer(buffer.data(), buffer.size());
        co_await m->recv_buffers(h);
        co_return buffer;
    }

    coro<void> read_address(const address& addr, std::span<char> buffer,
                            const std::vector<size_t>& offsets) override {
        auto m = co_await m_storage_service.acquire_messenger();

        co_await m->send_address(STORAGE_READ_ADDRESS_REQ, addr);
        const auto h = co_await m->recv_header();

        m->reserve_read_buffers(addr.size());
        for (size_t i = 0; i < addr.size(); ++i) {
            m->register_read_buffer(buffer.data() + offsets.at(i),
                                    addr.sizes[i]);
        }

        co_await m->recv_buffers(h);
    }

    coro<address> link(const address& addr) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send_address(STORAGE_LINK_REQ, addr);
        const auto message_header = co_await m->recv_header();
        co_return co_await m->recv_address(message_header);
    }

    coro<std::size_t> unlink(const address& addr) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send_address(STORAGE_UNLINK_REQ, addr);
        const auto message_header = co_await m->recv_header();
        co_return co_await m->recv_primitive<size_t>(message_header);
    }

    coro<std::size_t> get_used_space() override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send(STORAGE_USED_REQ, {});
        const auto message_header = co_await m->recv_header();
        co_return co_await m->recv_primitive<size_t>(message_header);
    }

    coro<std::map<size_t, size_t>> get_ds_size_map() override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send(STORAGE_DS_INFO_REQ, {});
        const auto message_header = co_await m->recv_header();
        co_return co_await m->recv_map<size_t, size_t>(message_header);
    }

    coro<void> ds_write(uint32_t ds_id, uint64_t pointer,
                        std::span<const char> data) override {
        auto m = co_await m_storage_service.acquire_messenger();
        ds_write_request req{.ds_id = ds_id, .pointer = pointer, .data = data};
        co_await m->send_ds_write(req);
        co_await m->recv_header();
    }

    coro<void> ds_read(uint32_t ds_id, uint64_t pointer, size_t size,
                       char* buffer) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send_ds_read(
            {.ds_id = ds_id, .pointer = pointer, .size = size});
        const auto header = co_await m->recv_header();
        if (header.size != size) {
            throw std::runtime_error(
                "mistmatched read size with requested size in ds_read");
        }
        m->register_read_buffer(buffer, size);
        co_await m->recv_buffers(header);
    }

private:
    client m_storage_service;
};

} // namespace uh::cluster
