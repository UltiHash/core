/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
