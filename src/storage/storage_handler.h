#ifndef CORE_DATA_STORE_SERVICE_HANDLER_H
#define CORE_DATA_STORE_SERVICE_HANDLER_H

#include "common/telemetry/metrics_handler.h"
#include "common/utils/common.h"
#include "common/utils/protocol_handler.h"
#include "data_store.h"
#include <utility>

namespace uh::cluster {

class storage_handler : public protocol_handler {
  public:
    storage_handler(storage_config config, size_t index,
                    std::shared_ptr<metrics_handler> metrics_handler)
        : m_data_store(std::move(config), index),
          m_metrics_handler(metrics_handler) {}

    coro<void> handle(boost::asio::ip::tcp::socket s) override {

        messenger m(std::move(s), m_metrics_handler);

        for (;;) {
            std::optional<error> err;

            try {
                const auto message_header = co_await m.recv_header();

                switch (message_header.type) {
                case STORAGE_WRITE_REQ:
                    co_await handle_write(m, message_header);
                    break;
                case STORAGE_READ_FRAGMENT_REQ:
                    co_await handle_read_fragment(m, message_header);
                    break;
                case STORAGE_READ_ADDRESS_REQ:
                    co_await handle_read_address(m, message_header);
                    break;
                case STORAGE_REMOVE_FRAGMENT_REQ:
                    co_await handle_remove_fragment(m, message_header);
                    break;
                case STORAGE_SYNC_REQ:
                    co_await handle_sync(m, message_header);
                    break;
                case STORAGE_USED_REQ:
                    co_await handle_get_used(m, message_header);
                    break;
                default:
                    throw std::invalid_argument("Invalid message type!");
                }
            } catch (const error_exception& e) {
                err = e.error();
            } catch (const std::exception& e) {
                err = error(error::unknown, e.what());
            }

            if (err) {
                co_await m.send_error(*err);
            }
        }
    }

  private:
    coro<void> handle_write(messenger& m, const messenger::header& h) {
        unique_buffer<char> data(h.size);
        m.register_read_buffer(data);
        co_await m.recv_buffers(h);
        const auto addr = m_data_store.write(data.get_span());
        co_await m.send_address(SUCCESS, addr);
    }

    coro<void> handle_read_fragment(messenger& m, const messenger::header& h) {
        const auto resp = co_await m.recv_fragment(h);
        unique_buffer<char> buffer(resp.size);
        const auto size =
            m_data_store.read(buffer.data(), resp.pointer, resp.size);
        co_await m.send(SUCCESS, {buffer.data(), size});
    }

    coro<void> handle_read_address(messenger& m, const messenger::header& h) {
        const auto resp = co_await m.recv_address(h);

        const auto read_size =
            std::accumulate(resp.sizes.cbegin(), resp.sizes.cend(), 0ul);

        unique_buffer<char> buffer(read_size);
        size_t offset = 0;
        for (size_t i = 0; i < resp.size(); i++) {
            const auto frag = resp.get_fragment(i);
            if (m_data_store.read(buffer.data() + offset, frag.pointer,
                                  frag.size) != frag.size) [[unlikely]] {
                throw std::runtime_error(
                    "Could not read the data with the given size");
            }
            offset += frag.size;
        }
        co_await m.send(SUCCESS, {buffer.data(), offset});
    }

    coro<void> handle_remove_fragment(messenger& m,
                                      const messenger::header& h) {
        const auto resp = co_await m.recv_fragment(h);
        m_data_store.remove(resp.pointer, resp.size);
        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_sync(messenger& m, const messenger::header& h) {
        m_data_store.sync();
        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_get_used(messenger& m, const messenger::header&) {
        const auto used = m_data_store.get_used_space();
        co_await m.send_uint128_t(SUCCESS, used);
    }

    uh::cluster::data_store m_data_store;
    std::shared_ptr<uh::cluster::metrics_handler> m_metrics_handler;
};

} // end namespace uh::cluster

#endif // CORE_DATA_STORE_SERVICE_HANDLER_H
