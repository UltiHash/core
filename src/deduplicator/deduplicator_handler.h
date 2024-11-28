#ifndef CORE_DEDUPE_NODE_HANDLER_H
#define CORE_DEDUPE_NODE_HANDLER_H

#include "common/network/messenger.h"
#include "common/utils/protocol_handler.h"
#include "deduplicator/interfaces/local_deduplicator.h"

namespace uh::cluster {

class deduplicator_handler : public protocol_handler {

public:
    explicit deduplicator_handler(local_deduplicator& local_dedupe);

    coro<void> handle(boost::asio::ip::tcp::socket s) override;

private:
    coro<void> handle_dedupe(context& ctx, messenger& m,
                             const messenger::header& h);

    local_deduplicator& m_local_dedupe;
};

} // end namespace uh::cluster

#endif // CORE_DEDUPE_NODE_HANDLER_H
