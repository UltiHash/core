#pragma once

#include <common/network/client.h>
#include <common/network/messenger_core.h>
#include <common/service_interfaces/deduplicator_interface.h>
#include <common/service_interfaces/service_factory.h>

namespace uh::cluster {

struct remote_deduplicator : public deduplicator_interface {
    explicit remote_deduplicator(client dedupe_service)
        : m_dedupe_service(std::move(dedupe_service)) {}

    coro<dedupe_response> deduplicate(std::string_view data) override {
        auto m = co_await m_dedupe_service.acquire_messenger();
        m->register_write_buffer(data);
        co_await m->send_buffers(DEDUPLICATOR_REQ);

        const auto h_dedupe = co_await m.get().recv_header();
        co_return co_await m->recv_dedupe_response(h_dedupe);
    }

private:
    client m_dedupe_service;
};

} // namespace uh::cluster
