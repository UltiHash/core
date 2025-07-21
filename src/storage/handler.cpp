#include "handler.h"

#include "config.h"
#include <common/telemetry/log.h>
#include <common/telemetry/metrics.h>
#include <common/utils/common.h>
#include <common/utils/pointer_traits.h>

#include <utility>

namespace uh::cluster::storage {

coro<void> handler::run() {
    std::stringstream remote;
    remote << m_messenger.peer();

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
    };
    LOG_INFO() << m_messenger.peer() << " expires itself";
}

coro<void> handler::handle_request(const messenger::header& hdr) {
    switch (hdr.type) {
    case STORAGE_WRITE_REQ:
        co_await handle_write(hdr);
        break;
    case STORAGE_READ_REQ:
        co_await handle_read(hdr);
        break;
    case STORAGE_READ_ADDRESS_REQ:
        co_await handle_read_address(hdr);
        break;
    case STORAGE_LINK_REQ:
        co_await handle_link(hdr);
        break;
    case STORAGE_UNLINK_REQ:
        co_await handle_unlink(hdr);
        break;
    case STORAGE_GET_REFCOUNTS_REQ:
        co_await handle_get_refcounts(hdr);
        break;
    case STORAGE_USED_REQ:
        co_await handle_get_used(hdr);
        break;
    case STORAGE_ALLOCATE_REQ:
        co_await handle_allocate(hdr);
        break;
    default:
        throw std::invalid_argument("Invalid message type!");
    }
}

coro<void> handler::handle_write(const messenger::header& h) {
    auto req = co_await m_messenger.recv_write(h);

    co_await m_storage.write(req.allocation, req.buffers, req.refcounts);
    co_await m_messenger.send(SUCCESS, {});
}

coro<void> handler::handle_read(const messenger::header& h) {
    const auto frag = co_await m_messenger.recv_fragment(h);

    auto buffer = co_await m_storage.read(frag.pointer, frag.size);

    co_await m_messenger.send(SUCCESS, buffer.span());
}

coro<void> handler::handle_read_address(const messenger::header& h) {
    const auto addr = co_await m_messenger.recv_address(h);

    unique_buffer<char> buffer(addr.data_size());

    std::vector<size_t> offsets;
    offsets.reserve(addr.size());
    size_t offset = 0;
    for (const auto frag : addr.fragments) {
        offsets.emplace_back(offset);
        offset += frag.size;
    }

    co_await m_storage.read_address(addr, buffer.span(), offsets);
    co_await m_messenger.send(SUCCESS, buffer.span());
}

coro<void> handler::handle_link(const messenger::header& h) {

    const auto refcounts = co_await m_messenger.recv_refcounts(h);
    auto rejected_stripes = co_await m_storage.link(refcounts);

    co_await m_messenger.send_refcounts(SUCCESS, rejected_stripes);
}

coro<void> handler::handle_unlink(const messenger::header& h) {

    const auto addr = co_await m_messenger.recv_refcounts(h);
    std::size_t freed_bytes = co_await m_storage.unlink(addr);

    co_await m_messenger.send_primitive<size_t>(SUCCESS, freed_bytes);
}

coro<void> handler::handle_get_refcounts(const messenger::header& h) {
    std::vector<std::size_t> stripe_ids(h.size / sizeof(std::size_t));
    m_messenger.register_read_buffer(stripe_ids);
    co_await m_messenger.recv_buffers(h);

    auto refcounts = co_await m_storage.get_refcounts(stripe_ids);
    co_await m_messenger.send_refcounts(SUCCESS, refcounts);
}

coro<void> handler::handle_get_used(const messenger::header&) {
    const auto used = co_await m_storage.get_used_space();
    co_await m_messenger.send_primitive<size_t>(SUCCESS, used);
}

coro<void> handler::handle_allocate(const messenger::header& h) {
    std::size_t size;
    std::size_t alignment;
    m_messenger.register_read_buffer(size);
    m_messenger.register_read_buffer(alignment);
    co_await m_messenger.recv_buffers(h);
    auto rv = co_await m_storage.allocate(size, alignment);
    co_await m_messenger.send_allocation(SUCCESS, rv);
}

} // namespace uh::cluster::storage
