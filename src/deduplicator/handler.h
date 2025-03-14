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

    notrace_coro<void> handle(boost::asio::ip::tcp::socket s) override;

private:
    enum class flow_control : uint8_t { BREAK, CONTINUE };
    coro<flow_control> handle_dedupe(const messenger::header& hdr,
                                     messenger& m);

    local_deduplicator& m_local_dedupe;
};

} // namespace uh::cluster::deduplicator
