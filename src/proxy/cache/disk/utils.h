#pragma once

#include <proxy/cache/disk/object.h>

#include <common/service_interfaces/deduplicator_interface.h>
#include <storage/global/data_view.h>

namespace uh::cluster::proxy::cache::disk::utils {

coro<void> erase(storage::global::global_data_view& data_view,
                 object_handle& h) {
    auto s = co_await data_view.unlink(h.addr);
    if (s != h.size()) {
        throw std::runtime_error("failed to delete object");
    }
}
coro<object_handle> create(deduplicator_interface& dedupe,
                           std::span<const char> sv) {
    auto resp =
        co_await dedupe.deduplicate(std::string_view{sv.data(), sv.size()});
    co_return object_handle(std::move(resp.addr));
}

coro<void> read(storage::global::global_data_view& storage,
                const object_handle& objh, std::span<char> sv) {
    co_await storage.read_address(objh.addr, sv);
}

} // namespace uh::cluster::proxy::cache::disk::utils
