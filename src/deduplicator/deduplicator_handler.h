#ifndef CORE_DEDUPE_NODE_HANDLER_H
#define CORE_DEDUPE_NODE_HANDLER_H

#include "common/utils/protocol_handler.h"
#include "config.h"
#include "fragment_set.h"

namespace uh::cluster {

class deduplicator_handler : public protocol_handler {

public:
    deduplicator_handler(deduplicator_config config, global_data_view& storage,
                         worker_pool& dedupe_workers);

    coro<void> handle(boost::asio::ip::tcp::socket s) override;

private:
    coro<void> handle_dedupe(messenger& m, const messenger::header& h);

    dedupe_response deduplicate(std::string_view data);

    deduplicator_config m_dedupe_conf;
    fragment_set m_fragment_set;
    global_data_view& m_storage;
    worker_pool& m_dedupe_workers;
};

} // end namespace uh::cluster

#endif // CORE_DEDUPE_NODE_HANDLER_H
