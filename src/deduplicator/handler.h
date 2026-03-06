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
#include "common/utils/protocol_handler.h"
#include "config.h"
#include "deduplicator/dedupe_set/fragment_set.h"
#include "deduplicator/interfaces/local_deduplicator.h"

namespace uh::cluster::deduplicator {

class handler : public protocol_handler {

public:
    explicit handler(local_deduplicator& local_dedupe);

    coro<void> handle(boost::asio::ip::tcp::socket s) override;

private:
    coro<void> handle_request(const messenger::header& hdr, messenger& m);

    local_deduplicator& m_local_dedupe;
};

} // namespace uh::cluster::deduplicator
