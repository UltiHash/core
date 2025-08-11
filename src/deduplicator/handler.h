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
