#pragma once

#include <common/network/client.h>
#include <common/network/messenger_core.h>
#include <common/service_interfaces/deduplicator_interface.h>
#include <common/service_interfaces/service_factory.h>

namespace uh::cluster {

struct remote_deduplicator : public deduplicator_interface {
    explicit remote_deduplicator(client dedupe_service)
        : m_dedupe_service(std::move(dedupe_service)) {}

    coro<dedupe_response> deduplicate(context& ctx,
                                      std::string_view data) override {
        auto m = co_await m_dedupe_service.acquire_messenger();
        m->register_write_buffer(data);
        co_await m->send_buffers(ctx, DEDUPLICATOR_REQ);

        const auto h_dedupe = co_await m.get().recv_header();
        co_return co_await m->recv_dedupe_response(h_dedupe);
    }

private:
    client m_dedupe_service;
};

template <>
std::shared_ptr<deduplicator_interface>
service_factory<deduplicator_interface>::make_remote_service(
    const std::string& hostname, uint16_t port) {
    return std::make_shared<remote_deduplicator>(
        client(m_ioc, hostname, port, m_connections));
}

} // namespace uh::cluster
