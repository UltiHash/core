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

#include "common/network/messenger.h"
#include <common/utils/protocol_handler.h>
#include <storage/group/storage_state_manager.h>
#include <storage/interfaces/local_storage.h>

namespace uh::cluster::storage {

class handler : public protocol_handler {
public:
    explicit handler(local_storage& storage);

    coro<void> handle(boost::asio::ip::tcp::socket s) override;

private:
    coro<void> handle_iteration(const messenger::header& hdr, messenger& m);

    coro<void> handle_write(messenger& m, const messenger::header& h);

    coro<void> handle_read(messenger& m, const messenger::header& h);

    coro<void> handle_read_address(messenger& m, const messenger::header& h);

    coro<void> handle_link(messenger& m, const messenger::header& h);

    coro<void> handle_unlink(messenger& m, const messenger::header& h);

    coro<void> handle_get_refcounts(messenger& m, const messenger::header& h);

    coro<void> handle_get_used(messenger& m, const messenger::header&);

    coro<void> handle_allocate(messenger& m, const messenger::header&);

    local_storage& m_storage;
};

} // namespace uh::cluster::storage
