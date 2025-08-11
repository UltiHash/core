#include "handler.h"

#include "common/utils/common.h"
#include "fragmentation.h"
#include <utility>

namespace uh::cluster::deduplicator {

coro<void> handler::run() {
    auto peer = m_messenger.peer();
    std::stringstream remote;
    remote << peer;

    auto state = co_await boost::asio::this_coro::cancellation_state;
    while (state.cancelled() == boost::asio::cancellation_type::none) {
        try {
            messenger_core::header hdr;
            opentelemetry::context::Context context;

            std::optional<error> err;

            try {
                LOG_DEBUG() << remote.str() << " waiting for request";
                std::tie(hdr, context) =
                    co_await m_messenger.recv_header_with_context();
                LOG_DEBUG() << remote.str() << " received "
                            << magic_enum::enum_name(hdr.type);

                boost::asio::context::set_pointer(context, "peer", &peer);

                co_await handle_request(hdr).continue_trace(std::move(context));

            } catch (const boost::system::system_error& e) {
                throw;
            } catch (const downstream_exception& e) {
                if (e.code() == boost::asio::error::operation_aborted) {
                    throw e.original_exception();
                } else if (e.code() == boost::beast::error::timeout) {
                    err = error(error::busy, e.what());
                } else {
                    err = error(error::internal_network_error, e.what());
                }
            } catch (const error_exception& e) {
                err = e.error();
            } catch (const std::exception& e) {
                err = error(error::unknown, e.what());
            }

            if (err) {
                LOG_WARN() << hdr.peer
                           << " error handling request: " << err->message();
                co_await m_messenger.send_error(*err);
            }

        } catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::operation_aborted or
                e.code() == boost::system::errc::bad_file_descriptor) {
                break;
            } else if (e.code() == boost::asio::error::eof) {
                LOG_INFO() << m_messenger.peer() << " disconnected";
                break;
            }
            throw;
        }
    }
    LOG_INFO() << m_messenger.peer() << " expired";
}

coro<void> handler::handle_request(const messenger::header& hdr) {
    std::optional<error> err;
    switch (hdr.type) {
    case DEDUPLICATOR_REQ: {
        unique_buffer<char> data(hdr.size);
        m_messenger.register_read_buffer(data);
        co_await m_messenger.recv_buffers(hdr);

        LOG_DEBUG() << hdr.peer << ": deduplicate: size=" << data.size();
        auto dedupe_resp = co_await m_dedup.deduplicate(data.string_view());
        co_await m_messenger.send_dedupe_response(dedupe_resp);
        break;
    }
    default:
        throw std::invalid_argument("Invalid message type!");
    }
}

} // namespace uh::cluster::deduplicator
