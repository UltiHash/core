
#ifndef UH_CLUSTER_REMOTE_STORAGE_H
#define UH_CLUSTER_REMOTE_STORAGE_H

#include "common/network/client.h"
#include "storage_interface.h"

namespace uh::cluster {

struct remote_storage : public storage_interface {

    explicit remote_storage(client storage_service)
        : m_storage_service(std::move(storage_service)) {}

    coro<address> write(const std::string_view& data) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m.get().send(STORAGE_WRITE_REQ, data);
        const auto message_header = co_await m.get().recv_header();
        co_return co_await m.get().recv_address(message_header);
    }

    coro<void> read_fragment(char* buffer, const fragment& frag) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m.get().send_fragment(STORAGE_READ_FRAGMENT_REQ, frag);
        const auto h = co_await m.get().recv_header();
        if (h.size != frag.size) [[unlikely]] {
            throw std::runtime_error("Incomplete fragment");
        }
        m.get().register_read_buffer(buffer, frag.size);
        co_await m.get().recv_buffers(h);
    }

    coro<void> read_address(char* buffer, const address& addr,
                            const std::vector<size_t>& offsets) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m.get().send_address(STORAGE_READ_ADDRESS_REQ, addr);
        const auto h = co_await m.get().recv_header();
        m.get().reserve_read_buffers(addr.size());
        for (size_t i = 0; i < addr.size(); ++i) {
            m.get().register_read_buffer(buffer + offsets.at(i), addr.sizes[i]);
        }
        co_await m.get().recv_buffers(h);
    }

    coro<void> sync(const address& addr) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m.get().send_address(STORAGE_SYNC_REQ, addr);
        co_await m.get().recv_header();
    }

    coro<size_t> get_used_space() override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m.get().send(STORAGE_USED_REQ, {});
        const auto message_header = co_await m.get().recv_header();
        co_return co_await m.get().recv_primitive<size_t>(message_header);
    }

    coro<size_t> get_free_space() override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m.get().send(STORAGE_AVAILABLE_REQ, {});
        const auto message_header = co_await m.get().recv_header();
        co_return co_await m.get().recv_primitive<size_t>(message_header);
    }

private:
    client m_storage_service;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_REMOTE_STORAGE_H
