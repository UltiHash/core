#pragma once

#include "common/network/messenger.h"
#include <common/etcd/registry/storage_registry.h>
#include <common/utils/protocol_handler.h>
#include <storage/interfaces/local_storage.h>

namespace uh::cluster::storage {

class handler : public protocol_handler {
public:
    explicit handler(local_storage& storage);

    coro<void> handle(boost::asio::ip::tcp::socket s) override;

private:
    enum class flow_control : uint8_t { BREAK, CONTINUE };
    coro<handler::flow_control> handle_iteration(const messenger::header& hdr,
                                                 messenger& m);
    coro<void> handle_write(messenger& m, const messenger::header& h);

    coro<void> handle_read(messenger& m, const messenger::header& h);

    coro<void> handle_read_address(messenger& m, const messenger::header& h);

    coro<void> handle_link(messenger& m, const messenger::header& h);

    coro<void> handle_unlink(messenger& m, const messenger::header& h);

    coro<void> handle_get_used(messenger& m, const messenger::header&);

    local_storage& m_storage;
};

} // namespace uh::cluster::storage
