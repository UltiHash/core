#include "deduplicator_handler.h"

#include "common/utils/common.h"
#include "fragmentation.h"
#include <utility>

namespace uh::cluster {

deduplicator_handler::deduplicator_handler(local_deduplicator& local_dedupe)
    : m_local_dedupe(local_dedupe) {}

coro<void> deduplicator_handler::handle(boost::asio::ip::tcp::socket s) {
    std::stringstream remote;
    remote << s.remote_endpoint();

    messenger m(std::move(s));

    for (;;) {
        std::optional<error> err;

        try {
            const auto message_header = co_await m.recv_header();

            LOG_DEBUG() << remote.str() << " received "
                        << magic_enum::enum_name(message_header.type);

            switch (message_header.type) {
            case DEDUPLICATOR_REQ:

                co_await handle_dedupe(m, message_header);
                break;
            default:
                throw std::invalid_argument("Invalid message type!");
            }
        } catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::eof) {
                LOG_INFO() << remote.str() << " disconnected";
                break;
            }
            err = error(error::unknown, e.what());
        } catch (const error_exception& e) {
            err = e.error();
        } catch (const std::exception& e) {
            err = error(error::unknown, e.what());
        }
        if (err) {
            LOG_WARN() << remote.str()
                       << " error handling request: " << err->message();
            co_await m.send_error(*err);
        }
    }
}

coro<void> deduplicator_handler::handle_dedupe(messenger& m,
                                               const messenger::header& h) {

    if (h.size == 0) [[unlikely]] {
        throw std::length_error("Empty data sent do the dedupe node");
    }

    unique_buffer<char> data(h.size);
    m.register_read_buffer(data);
    co_await m.recv_buffers(h);

    auto dedupe_resp = co_await m_local_dedupe.deduplicate(data.get_str_view());
    co_await m.send_dedupe_response(dedupe_resp);
}

} // end namespace uh::cluster
