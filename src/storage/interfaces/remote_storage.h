#pragma once

#include <storage/interface.h>
#include <storage/protocol.h>

namespace uh::cluster {

struct remote_storage : public sn::interface {

    explicit remote_storage(client storage_service)
        : m_storage_service(std::move(storage_service)) {}

    coro<address> write(context& ctx, std::span<const char> data,
                        const std::vector<std::size_t>& offsets) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_return co_await sn::write(m, ctx, data, offsets);
    }

    coro<std::size_t> read(context& ctx, const address& addr,
                           std::span<char> buffer) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_return co_await sn::read(m, ctx, addr, buffer);
    }

    coro<address> link(context& ctx, const address& addr) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_return co_await sn::link(m, ctx, addr);
    }

    coro<std::size_t> unlink(context& ctx, const address& addr) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_return co_await sn::unlink(m, ctx, addr);
    }

    coro<std::size_t> get_used_space(context& ctx) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_return co_await sn::get_used_space(m, ctx);
    }

    coro<std::map<size_t, size_t>> get_ds_size_map(context& ctx) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_return co_await sn::get_ds_size_map(m, ctx);
    }

    coro<void> ds_write(context& ctx, uint32_t ds_id, uint64_t pointer,
                        std::span<const char> data) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await sn::ds_write(m, ctx, ds_id, pointer, data);
    }

    coro<void> ds_read(context& ctx, uint32_t ds_id, uint64_t pointer,
                       size_t size, char* buffer) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await sn::ds_read(m, ctx, ds_id, pointer, size, buffer);
    }

private:
    client m_storage_service;
};

} // namespace uh::cluster
