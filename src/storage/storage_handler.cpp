#include "storage_handler.h"

#include "config.h"
#include "data_store.h"
#include <common/telemetry/log.h>
#include <common/telemetry/metrics.h>
#include <common/utils/common.h>
#include <common/utils/pointer_traits.h>

#include <utility>

namespace uh::cluster {

storage_handler::storage_handler(local_storage& storage)
    : m_storage(storage) {}

coro<void> storage_handler::handle(boost::asio::ip::tcp::socket s) {
    std::stringstream remote;
    remote << s.remote_endpoint();

    messenger m(std::move(s));

    for (;;) {

        context ctx;
        std::optional<error> err;

        try {
            const auto message_header = co_await m.recv_header();
            ctx = co_await m.recv_context(message_header);
            auto span =
                trace::scoped_span("received req", ctx.get_otel_context());

            LOG_DEBUG() << remote.str() << ": received "
                        << magic_enum::enum_name(message_header.type);

            switch (message_header.type) {
            case STORAGE_WRITE_REQ:
                co_await handle_write(ctx, m, message_header);
                break;
            case STORAGE_READ_REQ:
                co_await handle_read(ctx, m, message_header);
                break;
            case STORAGE_READ_FRAGMENT_REQ:
                co_await handle_read_fragment(ctx, m, message_header);
                break;
            case STORAGE_READ_ADDRESS_REQ:
                co_await handle_read_address(ctx, m, message_header);
                break;
            case STORAGE_LINK_REQ:
                co_await handle_link(ctx, m, message_header);
                break;
            case STORAGE_UNLINK_REQ:
                co_await handle_unlink(ctx, m, message_header);
                break;
            case STORAGE_USED_REQ:
                co_await handle_get_used(ctx, m, message_header);
                break;
            case STORAGE_DS_INFO_REQ:
                co_await handle_ds_info(ctx, m, message_header);
                break;
            case STORAGE_INIT_DD_REQ:
                co_await handle_init_dd(ctx, m, message_header);
                break;
            case STORAGE_DS_WRITE_REQ:
                co_await handle_ds_write(ctx, m, message_header);
                break;
            case STORAGE_DS_READ_REQ:
                co_await handle_ds_read(ctx, m, message_header);
                break;
            default:
                throw std::invalid_argument("Invalid message type!");
            }
        } catch (const boost::system::system_error& e) {
            LOG_ERROR() << "boost::system::system_error should be converted to "
                           "error_exception with error::internal_network_error";
            if (e.code() == boost::asio::error::eof) {
                LOG_INFO() << remote.str() << " disconnected";
                break;
            }
            err = error(error::unknown, e.what());
        } catch (const error_exception& e) {
            if (*e.error() == error::internal_network_error) {
                LOG_INFO() << remote.str() << " disconnected";
                break;
            }
            err = e.error();
        } catch (const std::exception& e) {
            err = error(error::unknown, e.what());
        }

        if (err) {
            LOG_WARN() << remote.str()
                       << " error handling request: " << err->message();
            co_await m.send_error(ctx, *err);
        }
    }
}

coro<void> storage_handler::handle_write(context& ctx, messenger& m,
                                         const messenger::header& h) {
    write_request req = co_await m.recv_write(h);
    auto addr = co_await m_storage.write(
        ctx, std::get<unique_buffer<>>(req.data).string_view(), req.offsets);
    co_await m.send_address(ctx, SUCCESS, addr);
}

coro<void> storage_handler::handle_read(context& ctx, messenger& m,
                                        const messenger::header& h) {
    const auto frag = co_await m.recv_fragment(h);

    auto buffer = co_await m_storage.read(ctx, frag.pointer, frag.size);

    co_await m.send(ctx, SUCCESS, buffer.span());
}

coro<void> storage_handler::handle_read_fragment(context& ctx, messenger& m,
                                                 const messenger::header& h) {
    const auto frag = co_await m.recv_fragment(h);

    unique_buffer<char> buffer(frag.size);
    co_await m_storage.read_fragment(ctx, buffer.data(), frag);

    co_await m.send(ctx, SUCCESS, buffer.span());
}

coro<void> storage_handler::handle_read_address(context& ctx, messenger& m,
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

    co_await m_storage.read_address(ctx, buffer.data(), addr, offsets);
    co_await m.send(ctx, SUCCESS, buffer.span());
}

coro<void> storage_handler::handle_link(context& ctx, messenger& m,
                                        const messenger::header& h) {

    const auto addr = co_await m.recv_address(h);
    auto rejected_addr = co_await m_storage.link(ctx, addr);

    co_await m.send_address(ctx, SUCCESS, rejected_addr);
}

coro<void> storage_handler::handle_unlink(context& ctx, messenger& m,
                                          const messenger::header& h) {

    const auto addr = co_await m.recv_address(h);
    std::size_t freed_bytes = co_await m_storage.unlink(ctx, addr);

    co_await m.send_primitive<size_t>(ctx, SUCCESS, freed_bytes);
}

coro<void> storage_handler::handle_get_used(context& ctx, messenger& m,
                                            const messenger::header&) {
    const auto used = co_await m_storage.get_used_space(ctx);
    co_await m.send_primitive<size_t>(ctx, SUCCESS, used);
}

coro<void> storage_handler::handle_ds_info(context& ctx, messenger& m,
                                           const messenger::header&) {
    const auto map = co_await m_storage.get_ds_size_map(ctx);
    co_await m.send_map(ctx, SUCCESS, map);
}

coro<void> storage_handler::handle_init_dd(context& ctx, messenger& m,
                                           const messenger::header&) {
    co_await m.send(ctx, SUCCESS, {});
}

coro<void> storage_handler::handle_ds_write(context& ctx, messenger& m,
                                            const messenger::header& h) {
    const auto req = co_await m.recv_ds_write(h);
    co_await m_storage.ds_write(
        ctx, req.ds_id, req.pointer,
        std::get<unique_buffer<>>(req.data).string_view());
    co_await m.send(ctx, SUCCESS, {});
}

coro<void> storage_handler::handle_ds_read(context& ctx, messenger& m,
                                           const messenger::header& h) {
    const auto req = co_await m.recv_ds_read(h);
    unique_buffer<> buf{req.size};
    co_await m_storage.ds_read(ctx, req.ds_id, req.pointer, req.size,
                               buf.data());
    co_await m.send(ctx, SUCCESS, buf.span());
}

} // namespace uh::cluster
