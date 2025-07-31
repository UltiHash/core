#pragma once

#include "common/network/messenger.h"
#include <common/utils/protocol_handler.h>
#include <storage/group/storage_state_manager.h>
#include <storage/interfaces/local_storage.h>

namespace uh::cluster::storage {

class handler_factory;

class handler : public protocol_handler {
public:
    explicit handler(boost::asio::ip::tcp::socket&& socket,
                     local_storage& storage)
        : m_messenger(std::move(socket), messenger::origin::UPSTREAM),
          m_storage{storage} {
        LOG_INFO() << "session started: " << m_messenger.peer();
    }

    coro<void> run() override;

private:
    messenger m_messenger;
    local_storage& m_storage;

    coro<void> handle_request(const messenger::header& hdr);
    coro<void> handle_write(const messenger::header& h);
    coro<void> handle_read(const messenger::header& h);
    coro<void> handle_read_address(const messenger::header& h);
    coro<void> handle_link(const messenger::header& h);
    coro<void> handle_unlink(const messenger::header& h);
    coro<void> handle_get_refcounts(const messenger::header& h);
    coro<void> handle_get_used(const messenger::header&);
    coro<void> handle_allocate(const messenger::header&);
};

class handler_factory : public protocol_handler_factory {
public:
    explicit handler_factory(local_storage& storage)
        : m_storage(storage) {}

    std::unique_ptr<protocol_handler>
    create_handler(boost::asio::ip::tcp::socket&& s) {
        return std::make_unique<handler>(std::move(s), m_storage);
    }

private:
    friend class handler;
    local_storage& m_storage;
};

} // namespace uh::cluster::storage
