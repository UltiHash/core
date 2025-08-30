/*
 * Disk utilities: APIs for common disk operations
 */
#pragma once

#include <common/service_interfaces/deduplicator_interface.h>
#include <common/types/common_types.h>
#include <storage/interfaces/data_view.h>

#include <memory>

namespace uh::cluster::proxy::cache::disk::utils {

inline coro<void> erase(storage::data_view& data_view, const address& addr) {
    co_await data_view.unlink(addr);
}

inline coro<address> store(deduplicator_interface& dedupe,
                           std::span<const char> sv) {
    auto resp =
        co_await dedupe.deduplicate(std::string_view{sv.data(), sv.size()});
    co_return std::move(resp.addr);
}

inline coro<void> read(storage::data_view& storage, const address& addr,
                       std::span<char> sv) {
    co_await storage.read_address(addr, sv);
}

} // namespace uh::cluster::proxy::cache::disk::utils
