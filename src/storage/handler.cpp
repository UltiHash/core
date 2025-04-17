#include "handler.h"

#include "config.h"
#include <common/telemetry/log.h>
#include <common/telemetry/metrics.h>
#include <common/utils/common.h>
#include <common/utils/pointer_traits.h>

#include <utility>

namespace uh::cluster::storage {

handler::handler(local_storage& storage, storage_registry& registry)
    : m_storage(storage),
      m_registry(registry) {}

coro<void> handler::handle(boost::asio::ip::tcp::socket s) {
    std::stringstream remote;
    remote << s.remote_endpoint();

    messenger m(std::move(s));

    for (;;) {
        std::optional<error> err;
        messenger_core::header hdr;
        opentelemetry::context::Context context;

        try {
            std::tie(hdr, context) = co_await m.recv_header_with_context();
            LOG_DEBUG() << remote.str() << " received "
                        << magic_enum::enum_name(hdr.type);

        } catch (const boost::system::system_error& e) {
            LOG_ERROR() << "boost::system::system_error should be converted to "
                           "error_exception with error::internal_network_error";
            if (e.code() == boost::asio::error::eof) {
                break;
            }
            err = error(error::unknown, e.what());
        } catch (const error_exception& e) {
            if (*e.error() == error::internal_network_error) {
                break;
            }
            err = e.error();
        } catch (const std::exception& e) {
            err = error(error::unknown, e.what());
        }

        if (err) {
            LOG_WARN() << remote.str()
                       << " error handling request: " << err->message();
        } else {
            if (co_await handle_iteration(hdr, m).continue_trace(
                    std::move(context)) == flow_control::BREAK) {
                break;
            }
        }
    };
}

coro<handler::flow_control>
handler::handle_iteration(const messenger::header& hdr, messenger& m) {
    std::optional<error> err;

    try {
        switch (hdr.type) {
        case STORAGE_WRITE_REQ:
            co_await handle_write(m, hdr);
            break;
        case STORAGE_READ_REQ:
            co_await handle_read(m, hdr);
            break;
        case STORAGE_READ_FRAGMENT_REQ:
            co_await handle_read_fragment(m, hdr);
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
        case STORAGE_USED_REQ:
            co_await handle_get_used(m, hdr);
            break;
        case STORAGE_UPDATE_STATE:
            co_await handle_update_state(m, hdr);
            break;
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
        co_await m.send_error(*err);
    }
    co_return flow_control::CONTINUE;
}

coro<void> handler::handle_write(messenger& m, const messenger::header& h) {
    write_request req = co_await m.recv_write(h);
    auto addr = co_await m_storage.write(
        std::get<unique_buffer<>>(req.data).string_view(), req.offsets);
    co_await m.send_address(SUCCESS, addr);
}

coro<void> handler::handle_read(messenger& m, const messenger::header& h) {
    const auto frag = co_await m.recv_fragment(h);

    auto buffer = co_await m_storage.read(frag.pointer, frag.size);

    co_await m.send(SUCCESS, buffer.span());
}

coro<void> handler::handle_read_fragment(messenger& m,
                                         const messenger::header& h) {
    const auto frag = co_await m.recv_fragment(h);

    unique_buffer<char> buffer(frag.size);
    co_await m_storage.read_fragment(buffer.data(), frag);

    co_await m.send(SUCCESS, buffer.span());
}

coro<void> handler::handle_read_address(messenger& m,
                                        const messenger::header& h) {
    const auto addr = co_await m.recv_address(h);

    unique_buffer<char> buffer(addr.data_size());

    std::vector<size_t> offsets;
    offsets.reserve(addr.size());
    size_t offset = 0;
    for (const auto fsize : addr.sizes) {
        offsets.emplace_back(offset);
        offset += fsize;
    }

    co_await m_storage.read_address(addr, buffer.span(), offsets);
    co_await m.send(SUCCESS, buffer.span());
}

coro<void> handler::handle_link(messenger& m, const messenger::header& h) {

    const auto addr = co_await m.recv_address(h);
    auto rejected_addr = co_await m_storage.link(addr);

    co_await m.send_address(SUCCESS, rejected_addr);
}

coro<void> handler::handle_unlink(messenger& m, const messenger::header& h) {

    const auto addr = co_await m.recv_address(h);
    std::size_t freed_bytes = co_await m_storage.unlink(addr);

    co_await m.send_primitive<size_t>(SUCCESS, freed_bytes);
}

coro<void> handler::handle_get_used(messenger& m, const messenger::header&) {
    const auto used = co_await m_storage.get_used_space();
    co_await m.send_primitive<size_t>(SUCCESS, used);
}

coro<void> handler::handle_update_state(messenger& m,
                                        const messenger::header& h) {
    const auto state = co_await m.recv_primitive<uint8_t>(h);
    const auto state_enum =
        magic_enum::enum_cast<storage_registry::storage_state>(state);
    if (state_enum.has_value()) {
        m_registry.update_service_state(state_enum.value());
    }
    co_await m.send(SUCCESS, {});
}

} // namespace uh::cluster::storage
