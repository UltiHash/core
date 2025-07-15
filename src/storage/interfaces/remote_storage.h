#pragma once

#include <common/network/client.h>
#include <common/service_interfaces/service_factory.h>
#include <common/service_interfaces/storage_interface.h>
#include <common/utils/strings.h>
#include <storage/group/state.h>

namespace uh::cluster {

struct remote_storage : public storage_interface {

    explicit remote_storage(client storage_service)
        : m_storage_service(std::move(storage_service)) {}

    coro<void> write(allocation_t allocation,
                     const std::vector<std::span<const char>>& buffers,
                     const std::vector<refcount_t>& refcounts) override {
        auto m = co_await m_storage_service.acquire_messenger();
        write_request_view req = {.allocation = allocation,
                                  .buffers = buffers,
                                  .refcounts = refcounts};

        co_await m->send_write(req);
        co_await m->recv_header(time_settings::instance().storage_timeout);
    }

    coro<shared_buffer<>> read(const storage_pointer& pointer,
                               size_t size) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send_fragment(STORAGE_READ_REQ, {pointer, size});
        const auto h =
            co_await m->recv_header(time_settings::instance().storage_timeout);
        shared_buffer<> buffer(h.size);
        m->register_read_buffer(buffer.data(), buffer.size());
        co_await m->recv_buffers(h);
        co_return buffer;
    }

    coro<void> read_address(const storage_address& addr, std::span<char> buffer,
                            const std::vector<size_t>& offsets) override {
        auto m = co_await m_storage_service.acquire_messenger();

        co_await m->send_address(STORAGE_READ_ADDRESS_REQ, addr);

        const auto h =
            co_await m->recv_header(time_settings::instance().storage_timeout);

        m->reserve_read_buffers(addr.size());
        for (size_t i = 0; i < addr.size(); ++i) {
            m->register_read_buffer(buffer.data() + offsets.at(i),
                                    addr.fragments[i].size);
        }

        co_await m->recv_buffers(h);
    }

    coro<std::vector<refcount_t>>
    link(const std::vector<refcount_t>& refcounts) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send_refcounts(STORAGE_LINK_REQ, refcounts);
        const auto h =
            co_await m->recv_header(time_settings::instance().storage_timeout);
        co_return co_await m->recv_refcounts(h);
    }

    coro<std::size_t>
    unlink(const std::vector<refcount_t>& refcounts) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send_refcounts(STORAGE_UNLINK_REQ, refcounts);
        const auto h =
            co_await m->recv_header(time_settings::instance().storage_timeout);
        co_return co_await m->recv_primitive<size_t>(h);
    }

    coro<std::size_t> get_used_space() override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m->send(STORAGE_USED_REQ, {});
        const auto h =
            co_await m->recv_header(time_settings::instance().storage_timeout);
        co_return co_await m->recv_primitive<size_t>(h);
    }

    coro<allocation_t> allocate(std::size_t size,
                                std::size_t alignment) override {
        auto m = co_await m_storage_service.acquire_messenger();
        m->register_write_buffer(size);
        m->register_write_buffer(alignment);
        co_await m->send_buffers(STORAGE_ALLOCATE_REQ);
        const auto h =
            co_await m->recv_header(time_settings::instance().storage_timeout);
        co_return co_await m->recv_allocation(h);
    }

    coro<std::vector<refcount_t>>
    get_refcounts(const std::vector<std::size_t>& stripe_ids) override {
        auto m = co_await m_storage_service.acquire_messenger();
        m->register_write_buffer(stripe_ids);
        co_await m->send_buffers(STORAGE_GET_REFCOUNTS_REQ);
        const auto h =
            co_await m->recv_header(time_settings::instance().storage_timeout);
        co_return co_await m->recv_refcounts(h);
    }

private:
    client m_storage_service;
};

} // namespace uh::cluster
