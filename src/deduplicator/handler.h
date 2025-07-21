#pragma once

#include "common/network/messenger.h"
#include "common/utils/protocol_handler.h"
#include "config.h"
#include "deduplicator/dedupe_set/fragment_set.h"
#include "deduplicator/interfaces/local_deduplicator.h"

namespace uh::cluster::deduplicator {

class handler : public protocol_handler {
public:
    explicit handler(boost::asio::ip::tcp::socket&& socket,
                     local_deduplicator& dedup)
        : m_messenger(std::move(socket), messenger::origin::UPSTREAM),
          m_dedup{dedup} {
        LOG_INFO() << "session started: " << m_messenger.peer();
    }
    ~handler() { LOG_INFO() << "session ended: " << m_messenger.peer(); }

    void cancel() override {
        auto& socket = m_messenger.get_socket();
        if (socket.is_open()) {
            try {
                LOG_DEBUG() << "socket canceled";
                socket.cancel();
            } catch (const boost::system::system_error& e) {
                LOG_ERROR() << "cancel failed: " << e.what();
            }
        } else {
            LOG_ERROR() << "socket is not open, cancel skipped.";
        }
    }

    coro<void> run() override;

private:
    messenger m_messenger;
    local_deduplicator& m_dedup;

    coro<void> handle_request(const messenger::header& hdr);
};

class handler_factory : public protocol_handler_factory {
public:
    explicit handler_factory(local_deduplicator& dedup)
        : m_dedup(dedup) {}

    std::unique_ptr<protocol_handler>
    create_handler(boost::asio::ip::tcp::socket&& s) {
        return std::make_unique<handler>(std::move(s), m_dedup);
    }

private:
    friend class handler;
    local_deduplicator& m_dedup;
};

} // namespace uh::cluster::deduplicator
