#include "handler.h"

#include "common/utils/common.h"
#include "fragmentation.h"
#include <utility>

namespace uh::cluster::deduplicator {

handler::handler(local_deduplicator& local_dedupe)
    : m_local_dedupe(local_dedupe) {}

notrace_coro<void> handler::handle(boost::asio::ip::tcp::socket s) {
    std::stringstream remote;
    remote << s.remote_endpoint();

    messenger m(std::move(s));

    bool should_continue = true;
    do {
        std::optional<error> err;
        messenger_header hdr;

        try {
            hdr = co_await m.recv_header();
            LOG_DEBUG() << remote.str() << " received "
                        << magic_enum::enum_name(hdr.type);

        } catch (const boost::system::system_error& e) {
            LOG_ERROR() << "boost::system::system_error should be converted to "
                           "error_exception with error::internal_network_error";
            if (e.code() == boost::asio::error::eof) {
                should_continue = false;
            }
            err = error(error::unknown, e.what());
        } catch (const error_exception& e) {
            if (*e.error() == error::internal_network_error) {
                should_continue = false;
            }
            err = e.error();
        } catch (const std::exception& e) {
            err = error(error::unknown, e.what());
        }

        if (err) {
            LOG_WARN() << remote.str()
                       << " error handling request: " << err->message();
        } else {
            if (co_await handle_dedupe(hdr, m) == flow_control::BREAK) {
                break;
            }
        }
    } while (should_continue);
}

coro<handler::flow_control> handler::handle_dedupe(const messenger::header& hdr,
                                                   messenger& m) {

    auto ctx = hdr.ctx;
    std::optional<error> err;

    try {
        switch (hdr.type) {
        case DEDUPLICATOR_REQ: {
            if (hdr.size == 0) [[unlikely]] {
                throw std::length_error("Empty data sent do the dedupe node");
            }

            unique_buffer<char> data(hdr.size);
            m.register_read_buffer(data);
            co_await m.recv_buffers(hdr);

            LOG_DEBUG() << hdr.peer << ": deduplicate: size=" << data.size();
            auto dedupe_resp =
                co_await m_local_dedupe.deduplicate(ctx, data.string_view());
            co_await m.send_dedupe_response(ctx, dedupe_resp);
            break;
        }
        default:
            throw std::invalid_argument("Invalid message type!");
        }
    } catch (const boost::system::system_error& e) {
        LOG_ERROR() << "boost::system::system_error should be converted to "
                       "error_exception with error::internal_network_error";
        if (e.code() == boost::asio::error::eof) {
            LOG_INFO() << hdr.peer << " disconnected";
            co_return flow_control::BREAK;
        }
        err = error(error::unknown, e.what());
    } catch (const error_exception& e) {
        if (*e.error() == error::internal_network_error) {
            LOG_INFO() << hdr.peer << " disconnected";
            co_return flow_control::BREAK;
        }
        err = e.error();
    } catch (const std::exception& e) {
        err = error(error::unknown, e.what());
    }

    if (err) {
        LOG_WARN() << hdr.peer << " error handling request: " << err->message();
        co_await m.send_error(ctx, *err);
    }
    co_return flow_control::CONTINUE;
}

} // namespace uh::cluster::deduplicator
