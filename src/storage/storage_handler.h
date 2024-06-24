#ifndef CORE_DATA_STORE_SERVICE_HANDLER_H
#define CORE_DATA_STORE_SERVICE_HANDLER_H

#include "common/debug/debug.h"
#include "common/telemetry/metrics.h"
#include "common/utils/common.h"
#include "common/utils/pointer_traits.h"

#include "common/utils/protocol_handler.h"
#include "config.h"
#include "data_store.h"
#include "storage/interfaces/local_storage.h"
#include <utility>

namespace uh::cluster {

class storage_handler : public protocol_handler {
public:
    explicit storage_handler(local_storage& storage)
        : m_storage(storage) {}

    coro<void> handle(boost::asio::ip::tcp::socket s) override {
        std::stringstream remote;
        remote << s.remote_endpoint();

        messenger m(std::move(s));

        for (;;) {
            std::optional<error> err;

            try {
                const auto message_header = co_await m.recv_header();
                LOG_CORO_CONTEXT();

                LOG_DEBUG() << remote.str() << " received "
                            << magic_enum::enum_name(message_header.type);

                switch (message_header.type) {
                case STORAGE_WRITE_REQ:
                    co_await handle_write(m, message_header);
                    break;
                case STORAGE_READ_REQ:
                    co_await handle_read(m, message_header);
                    break;
                case STORAGE_READ_FRAGMENT_REQ:
                    co_await handle_read_fragment(m, message_header);
                    break;
                case STORAGE_READ_ADDRESS_REQ:
                    co_await handle_read_address(m, message_header);
                    break;
                case STORAGE_SYNC_REQ:
                    co_await handle_sync(m, message_header);
                    break;
                case STORAGE_USED_REQ:
                    co_await handle_get_used(m, message_header);
                    break;
                case STORAGE_AVAILABLE_REQ:
                    co_await handle_get_available(m, message_header);
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

private:
    coro<void> handle_write(messenger& m, const messenger::header& h) {
        unique_buffer<char> data(h.size);
        m.register_read_buffer(data);
        co_await m.recv_buffers(h);
        auto addr = co_await m_storage.write(data.string_view());
        co_await m.send_address(SUCCESS, addr);
    }

    coro<void> handle_read(messenger& m, const messenger::header& h) {
        const auto frag = co_await m.recv_fragment(h);

        auto buffer = co_await m_storage.read(frag.pointer, frag.size);

        co_await m.send(SUCCESS, buffer.span());
    }

    coro<void> handle_read_fragment(messenger& m, const messenger::header& h) {
        const auto frag = co_await m.recv_fragment(h);

        unique_buffer<char> buffer(frag.size);
        co_await m_storage.read_fragment(buffer.data(), frag);

        co_await m.send(SUCCESS, buffer.span());
    }

    coro<void> handle_read_address(messenger& m, const messenger::header& h) {
        const auto addr = co_await m.recv_address(h);

        unique_buffer<char> buffer(addr.data_size());

        std::vector<size_t> offsets;
        offsets.reserve(addr.size());
        size_t offset = 0;
        for (const auto fsize : addr.sizes) {
            offsets.emplace_back(offset);
            offset += fsize;
        }

        co_await m_storage.read_address(buffer.data(), addr, offsets);
        co_await m.send(SUCCESS, buffer.span());
    }

    coro<void> handle_sync(messenger& m, const messenger::header& h) {

        const auto addr = co_await m.recv_address(h);
        co_await m_storage.sync(addr);

        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_get_used(messenger& m, const messenger::header&) {
        const auto used = co_await m_storage.get_used_space();
        co_await m.send_primitive<size_t>(SUCCESS, used);
    }

    coro<void> handle_get_available(messenger& m, const messenger::header&) {
        const auto available = co_await m_storage.get_free_space();
        co_await m.send_primitive<size_t>(SUCCESS, available);
    }

    local_storage& m_storage;
};

} // end namespace uh::cluster

#endif // CORE_DATA_STORE_SERVICE_HANDLER_H
