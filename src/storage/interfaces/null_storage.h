#pragma once

#include <storage/interface.h>

namespace uh::cluster {

struct null_storage : public sn::interface {

    coro<address> write(context& ctx, std::span<const char> data,
                        const std::vector<std::size_t>& offsets) override {
        co_return address{};
    }

    coro<std::size_t> read(context& ctx, const address& addr,
                           std::span<char> buffer) override {
        co_return 0ull;
    }

    coro<address> link(context& ctx, const address& addr) override {
        co_return address{};
    }

    coro<std::size_t> unlink(context& ctx, const address& addr) override {
        co_return 0ull;
    }

    coro<size_t> get_used_space(context& ctx) override { co_return 0ull; }

    coro<std::map<size_t, size_t>> get_ds_size_map(context& ctx) override {
        co_return std::map<std::size_t, std::size_t>{};
    }

    coro<void> ds_write(context& ctx, uint32_t ds_id, uint64_t pointer,
                        std::span<const char> data) override {
        co_return;
    }

    coro<void> ds_read(context& ctx, uint32_t ds_id, uint64_t pointer,
                       size_t size, char* buffer) override {
        co_return;
    }

    size_t get_free_space() { return 0ull; }
};

} // namespace uh::cluster
