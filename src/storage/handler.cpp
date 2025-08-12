#include "handler.h"

#include "config.h"
#include <common/telemetry/log.h>
#include <common/telemetry/metrics.h>
#include <common/utils/common.h>
#include <common/utils/pointer_traits.h>

#include <utility>

namespace uh::cluster::storage {

handler::handler(local_storage& storage)
    : m_storage(storage) {}

coro<void> handler::handle(boost::asio::ip::tcp::socket s) {
    std::stringstream remote;
    remote << s.remote_endpoint();

    messenger m(std::move(s), messenger::origin::UPSTREAM);

    auto state = co_await boost::asio::this_coro::cancellation_state;
    while (state.cancelled() == boost::asio::cancellation_type::none) {
        try {
            messenger_core::header hdr;
            opentelemetry::context::Context context;

            std::optional<error> err;

            try {
                LOG_DEBUG() << remote.str() << " waiting for request";
                std::tie(hdr, context) = co_await m.recv_header_with_context();
                LOG_DEBUG() << remote.str() << " received "
                            << magic_enum::enum_name(hdr.type);

                co_await handle_iteration(hdr, m).continue_trace(
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
                LOG_WARN() << hdr.peer
                           << " error handling request: " << err->message();
                co_await m.send_error(*err);
            }

        } catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::operation_aborted) {
                break;
            } else if (e.code() == boost::asio::error::eof) {
                LOG_INFO() << m.peer() << " disconnected";
                break;
            }
            throw;
        }
    };
    LOG_INFO() << m.peer() << " expired";
}

coro<void> handler::handle_iteration(const messenger::header& hdr,
                                     messenger& m) {
    switch (hdr.type) {
    case STORAGE_WRITE_REQ:
        co_await handle_write(m, hdr);
        break;
    case STORAGE_READ_REQ:
        co_await handle_read(m, hdr);
        break;
    case STORAGE_READ_ADDRESS_REQ:
        co_await handle_read_address(m, hdr);
        break;
    case STORAGE_LINK_REQ:
        co_await handle_link(m, hdr);
        break;
    case STORAGE_UNLINK_REQ:
        co_await handle_unlink(m, hdr);
        break;
    case STORAGE_GET_REFCOUNTS_REQ:
        co_await handle_get_refcounts(m, hdr);
        break;
    case STORAGE_USED_REQ:
        co_await handle_get_used(m, hdr);
        break;
    case STORAGE_ALLOCATE_REQ:
        co_await handle_allocate(m, hdr);
        break;
    default:
        throw std::invalid_argument("Invalid message type!");
    }
}

coro<void> handler::handle_write(messenger& m, const messenger::header& h) {
    auto req = co_await m.recv_write(h);

    co_await m_storage.write(req.allocation, req.buffers, req.refcounts);
    co_await m.send(SUCCESS, {});
}

coro<void> handler::handle_read(messenger& m, const messenger::header& h) {
    const auto frag = co_await m.recv_fragment(h);

    auto buffer = co_await m_storage.read(frag.pointer, frag.size);

    co_await m.send(SUCCESS, buffer.span());
}

coro<void> handler::handle_read_address(messenger& m,
                                        const messenger::header& h) {
    const auto addr = co_await m.recv_address(h);

    unique_buffer<char> buffer(addr.data_size());

    std::vector<size_t> offsets;
    offsets.reserve(addr.size());
    size_t offset = 0;
    for (const auto frag : addr.fragments) {
        offsets.emplace_back(offset);
        offset += frag.size;
    }

    co_await m_storage.read_address(addr, buffer.span(), offsets);
    co_await m.send(SUCCESS, buffer.span());
}

coro<void> handler::handle_link(messenger& m, const messenger::header& h) {

    const auto refcounts = co_await m.recv_refcounts(h);
    auto rejected_stripes = co_await m_storage.link(refcounts);

    co_await m.send_refcounts(SUCCESS, rejected_stripes);
}

coro<void> handler::handle_unlink(messenger& m, const messenger::header& h) {

    const auto addr = co_await m.recv_refcounts(h);
    std::size_t freed_bytes = co_await m_storage.unlink(addr);

    co_await m.send_primitive<size_t>(SUCCESS, freed_bytes);
}

coro<void> handler::handle_get_refcounts(uh::cluster::messenger& m,
                                         const messenger::header& h) {
    std::vector<std::size_t> stripe_ids(h.size / sizeof(std::size_t));
    m.register_read_buffer(stripe_ids);
    co_await m.recv_buffers(h);

    auto refcounts = co_await m_storage.get_refcounts(stripe_ids);
    co_await m.send_refcounts(SUCCESS, refcounts);
}

coro<void> handler::handle_get_used(messenger& m, const messenger::header&) {
    const auto used = co_await m_storage.get_used_space();
    co_await m.send_primitive<size_t>(SUCCESS, used);
}

coro<void> handler::handle_allocate(messenger& m, const messenger::header& h) {
    std::size_t size;
    std::size_t alignment;
    m.register_read_buffer(size);
    m.register_read_buffer(alignment);
    co_await m.recv_buffers(h);
    auto rv = co_await m_storage.allocate(size, alignment);
    co_await m.send_allocation(SUCCESS, rv);
}

} // namespace uh::cluster::storage
