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

    messenger m(std::move(s));

    for (;;) {

        context ctx;
        std::optional<error> err;

        try {
            auto hdr = co_await m.recv_header();
            ctx = std::move(hdr.ctx);

            LOG_DEBUG() << remote.str() << ": received "
                        << magic_enum::enum_name(hdr.type);

            switch (hdr.type) {
            case STORAGE_WRITE_REQ:
                ctx = ctx.sub_context("storage-write-req");
                co_await handle_write(ctx, m, hdr);
                break;
            case STORAGE_READ_REQ:
                ctx = ctx.sub_context("storage-read-req");
                co_await handle_read(ctx, m, hdr);
                break;
            case STORAGE_READ_FRAGMENT_REQ:
                ctx = ctx.sub_context("storage-read-fragment-req");
                co_await handle_read_fragment(ctx, m, hdr);
                break;
            case STORAGE_READ_ADDRESS_REQ:
                ctx = ctx.sub_context("storage-read-address-req");
                co_await handle_read_address(ctx, m, hdr);
                break;
            case STORAGE_LINK_REQ:
                ctx = ctx.sub_context("storage-link-req");
                co_await handle_link(ctx, m, hdr);
                break;
            case STORAGE_UNLINK_REQ:
                ctx = ctx.sub_context("storage-unlink-req");
                co_await handle_unlink(ctx, m, hdr);
                break;
            case STORAGE_USED_REQ:
                ctx = ctx.sub_context("storage-used-req");
                co_await handle_get_used(ctx, m, hdr);
                break;
            case STORAGE_DS_INFO_REQ:
                ctx = ctx.sub_context("storage-ds-info-req");
                co_await handle_ds_info(ctx, m, hdr);
                break;
            case STORAGE_INIT_DD_REQ:
                ctx = ctx.sub_context("storage-init-dd-req");
                co_await handle_init_dd(ctx, m, hdr);
                break;
            case STORAGE_DS_WRITE_REQ:
                ctx = ctx.sub_context("storage-ds-write-req");
                co_await handle_ds_write(ctx, m, hdr);
                break;
            case STORAGE_DS_READ_REQ:
                ctx = ctx.sub_context("storage-ds-read-req");
                co_await handle_ds_read(ctx, m, hdr);
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

coro<void> handler::handle_write(context& ctx, messenger& m,
                                 const messenger::header& h) {
    write_request req = co_await m.recv_write(h);
    auto addr = co_await m_storage.write(
        ctx, std::get<unique_buffer<>>(req.data).string_view(), req.offsets);
    co_await m.send_address(ctx, SUCCESS, addr);
}

coro<void> handler::handle_read(context& ctx, messenger& m,
                                const messenger::header& h) {
    const auto frag = co_await m.recv_fragment(h);

    unique_buffer<char> buffer(frag.size);
    auto size = co_await m_storage.read(ctx, frag, buffer.span());
    buffer.resize(size);

    co_await m.send(ctx, SUCCESS, buffer.span());
}

coro<void> handler::handle_read_fragment(context& ctx, messenger& m,
                                         const messenger::header& h) {
    return handle_read(ctx, m, h);
}

coro<void> handler::handle_read_address(context& ctx, messenger& m,
                                        const messenger::header& h) {
    const auto addr = co_await m.recv_address(h);

    unique_buffer<char> buffer(addr.data_size());
    auto count = co_await m_storage.read_address(ctx, addr, buffer.span());
    buffer.resize(count);

    co_await m.send(ctx, SUCCESS, buffer.span());
}

coro<void> handler::handle_link(context& ctx, messenger& m,
                                const messenger::header& h) {

    const auto addr = co_await m.recv_address(h);
    auto rejected_addr = co_await m_storage.link(ctx, addr);

    co_await m.send_address(ctx, SUCCESS, rejected_addr);
}

coro<void> handler::handle_unlink(context& ctx, messenger& m,
                                  const messenger::header& h) {

    const auto addr = co_await m.recv_address(h);
    std::size_t freed_bytes = co_await m_storage.unlink(ctx, addr);

    co_await m.send_primitive<size_t>(ctx, SUCCESS, freed_bytes);
}

coro<void> handler::handle_get_used(context& ctx, messenger& m,
                                    const messenger::header&) {
    const auto used = co_await m_storage.get_used_space(ctx);
    co_await m.send_primitive<size_t>(ctx, SUCCESS, used);
}

coro<void> handler::handle_ds_info(context& ctx, messenger& m,
                                   const messenger::header&) {
    const auto map = co_await m_storage.get_ds_size_map(ctx);
    co_await m.send_map(ctx, SUCCESS, map);
}

coro<void> handler::handle_init_dd(context& ctx, messenger& m,
                                   const messenger::header&) {
    co_await m.send(ctx, SUCCESS, {});
}

coro<void> handler::handle_ds_write(context& ctx, messenger& m,
                                    const messenger::header& h) {
    const auto req = co_await m.recv_ds_write(h);
    co_await m_storage.ds_write(
        ctx, req.ds_id, req.pointer,
        std::get<unique_buffer<>>(req.data).string_view());
    co_await m.send(ctx, SUCCESS, {});
}

coro<void> handler::handle_ds_read(context& ctx, messenger& m,
                                   const messenger::header& h) {
    const auto req = co_await m.recv_ds_read(h);
    unique_buffer<> buf{req.size};
    co_await m_storage.ds_read(ctx, req.ds_id, req.pointer, req.size,
                               buf.data());
    co_await m.send(ctx, SUCCESS, buf.span());
}

} // namespace uh::cluster::storage
