#include "handler.h"

#include "common/utils/common.h"
#include "fragmentation.h"
#include <utility>

namespace uh::cluster::deduplicator {

handler::handler(local_deduplicator& local_dedupe)
    : m_local_dedupe(local_dedupe) {}

coro<void> handler::handle(boost::asio::ip::tcp::socket s) {
    messenger m(std::move(s), messenger::origin::UPSTREAM);
    auto peer = m.peer();

    auto state = co_await boost::asio::this_coro::cancellation_state;
    while (state.cancelled() == boost::asio::cancellation_type::none) {
        try {
            messenger_core::header hdr;
            opentelemetry::context::Context context;

            std::optional<error> err;

            try {
                std::tie(hdr, context) = co_await m.recv_header_with_context();
                LOG_DEBUG() << peer << ": received "
                            << magic_enum::enum_name(hdr.type);

                boost::asio::context::set_pointer(context, "peer", &peer);

                co_await handle_request(hdr, m).continue_trace(
                    std::move(context));

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
                LOG_WARN() << peer
                           << " error handling request: " << err->message();
                co_await m.send_error(*err);
            }

        } catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::operation_aborted or
                e.code() == boost::system::errc::bad_file_descriptor) {
                break;
            } else if (e.code() == boost::asio::error::eof) {
                LOG_INFO() << peer << " disconnected";
                break;
            }
            throw;
        }
    }
    LOG_INFO() << m.peer() << " expired";
}

coro<void> handler::handle_request(const messenger::header& hdr, messenger& m) {
    std::optional<error> err;
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
}

} // namespace uh::cluster::deduplicator
