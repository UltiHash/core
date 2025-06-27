#include "handler.h"

#include "common/utils/common.h"
#include "fragmentation.h"
#include <utility>

namespace uh::cluster::deduplicator {

handler::handler(local_deduplicator& local_dedupe)
    : m_local_dedupe(local_dedupe) {}

coro<void> handler::handle(boost::asio::ip::tcp::socket s) {
    std::stringstream remote;
    remote << s.remote_endpoint();

    messenger m(std::move(s), connection_exception::origin::upstream);

    for (;;) {
        std::optional<error> err;
        messenger_core::header hdr;
        opentelemetry::context::Context context;

        std::tie(hdr, context) = co_await m.recv_header_with_context();
        LOG_DEBUG() << remote.str() << " received "
                    << magic_enum::enum_name(hdr.type);

        try {
            co_await handle_dedupe(hdr, m).continue_trace(std::move(context));
        } catch (const std::exception& e) {
            LOG_WARN() << remote.str()
                       << " error handling request: " << e.what();
            break;
        }
    }
}

coro<void> handler::handle_dedupe(const messenger::header& hdr, messenger& m) {
    std::optional<error> err;
    try {
        switch (hdr.type) {
        case DEDUPLICATOR_REQ: {
            unique_buffer<char> data(hdr.size);
            m.register_read_buffer(data);
            co_await m.recv_buffers(hdr);

            LOG_DEBUG() << hdr.peer << ": deduplicate: size=" << data.size();
            auto dedupe_resp =
                co_await m_local_dedupe.deduplicate(data.string_view());
            co_await m.send_dedupe_response(dedupe_resp);
            break;
        }
        default:
            throw std::invalid_argument("Invalid message type!");
        }
    } catch (const connection_exception& ce) {
        auto e = ce.original_exception();
        if (ce.get_origin() == connection_exception::origin::upstream) {
            if (e.code() != boost::asio::error::eof) {
                LOG_WARN() << "connection exception from upstream detected: "
                           << ce.what();
            }
            throw;
        } else {
            if (e.code() == boost::asio::error::operation_aborted or
                e.code() == boost::beast::error::timeout) {
                err = error(error::busy, e.what());
            } else {
                err = error(error::internal_network_error, e.what());
            }
        }
    } catch (const boost::system::system_error& e) {
        LOG_FATAL() << "boost::system::system_error should be converted to "
                       "error_exception with error::internal_network_error";
        throw;
    } catch (const error_exception& e) {
        if (*e.error() == error::internal_network_error) {
            throw;
        }
        err = e.error();
    } catch (const std::exception& e) {
        err = error(error::unknown, e.what());
    }

    if (err) {
        LOG_WARN() << hdr.peer << " error handling request: " << err->message();
        co_await m.send_error(*err);
    }
}

} // namespace uh::cluster::deduplicator
