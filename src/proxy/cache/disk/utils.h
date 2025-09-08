/*
 * Disk utilities: APIs for common disk operations
 */
#pragma once

#include <common/service_interfaces/deduplicator_interface.h>
#include <common/types/common_types.h>
#include <storage/interfaces/data_view.h>

#include <memory>

namespace uh::cluster::proxy::cache::disk::utils {

inline coro<void> erase(storage::data_view& storage, const address& addr) {
    co_await storage.unlink(addr);
}

inline coro<address> store(storage::data_view& storage,
                           std::span<const char> sv) {
    auto addr =
        co_await storage.write(std::string_view{sv.data(), sv.size()}, {0});
    co_return std::move(addr);
}

inline coro<void> read(storage::data_view& storage, const address& addr,
                       std::span<char> sv) {
    co_await storage.read_address(addr, sv);
}

} // namespace uh::cluster::proxy::cache::disk::utils
