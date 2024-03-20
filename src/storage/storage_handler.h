#ifndef CORE_DATA_STORE_SERVICE_HANDLER_H
#define CORE_DATA_STORE_SERVICE_HANDLER_H

#include "common/telemetry/metrics.h"
#include "common/utils/common.h"
#include "common/utils/protocol_handler.h"
#include "config.h"
#include "data_store.h"
#include <exception>
#include <utility>

namespace uh::cluster {

class storage_handler : public protocol_handler {
public:
    storage_handler(data_store_config config, size_t index,
                    boost::asio::io_context& ioc)
        : m_data_store(std::move(config), index),
          m_pool(4),
          m_ioc(ioc) {}

    coro<void> handle(boost::asio::ip::tcp::socket s) override {
        std::stringstream remote;
        remote << s.remote_endpoint();

        messenger m(std::move(s));

        for (;;) {
            std::optional<error> err;

            try {
                const auto message_header = co_await m.recv_header();

                LOG_DEBUG() << remote.str() << " received "
                            << magic_enum::enum_name(message_header.type);

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

        auto promise = std::make_shared<awaitable_promise<address>>(m_ioc);
        boost::asio::post(m_pool, [&, this]() {
            try {
                promise->set(m_data_store.write(data.get_span()));
            } catch (const std::exception& e) {
                promise->set_exception(std::current_exception());
            }
        });

        LOG_DEBUG() << "handle_write: waiting for promise";
        address addr = co_await promise->get();
        co_await m.send_address(SUCCESS, addr);
        LOG_DEBUG() << "handle_write: return";
    }

    coro<void> handle_read_fragment(messenger& m, const messenger::header& h) {
        const auto resp = co_await m.recv_fragment(h);
        unique_buffer<char> buffer(resp.size);

        auto promise = std::make_shared<awaitable_promise<std::size_t>>(m_ioc);
        boost::asio::post(m_pool, [&, this]() {
            try {
                promise->set(
                    m_data_store.read(buffer.data(), resp.pointer, resp.size));
            } catch (const std::exception& e) {
                promise->set_exception(std::current_exception());
            }
        });

        LOG_DEBUG() << "handle_read_fragment: waiting for promise";
        std::size_t size = co_await promise->get();
        co_await m.send(SUCCESS, {buffer.data(), size});
        LOG_DEBUG() << "handle_read_fragment: return";
    }

    coro<void> handle_read_address(messenger& m, const messenger::header& h) {
        const auto resp = co_await m.recv_address(h);

        const auto read_size =
            std::accumulate(resp.sizes.cbegin(), resp.sizes.cend(), 0ul);

        unique_buffer<char> buffer(read_size);

        auto promise = std::make_shared<awaitable_promise<std::size_t>>(m_ioc);
        boost::asio::post(m_pool, [&, this]() {
            try {
                size_t offset = 0;
                for (size_t i = 0; i < resp.size(); i++) {
                    const auto frag = resp.get_fragment(i);
                    if (m_data_store.read(buffer.data() + offset, frag.pointer,
                                          frag.size) != frag.size)
                        [[unlikely]] {
                        throw std::runtime_error(
                            "Could not read the data with the given size");
                    }
                    offset += frag.size;
                }

                promise->set(std::move(offset));

            } catch (const std::exception& e) {
                promise->set_exception(std::current_exception());
            }
        });

        LOG_DEBUG() << "handle_read_address: waiting for promise";
        std::size_t offset = co_await promise->get();
        co_await m.send(SUCCESS, {buffer.data(), offset});
        LOG_DEBUG() << "handle_read_address: return";
    }

    coro<void> handle_remove_fragment(messenger& m,
                                      const messenger::header& h) {
        const auto resp = co_await m.recv_fragment(h);
        m_data_store.remove(resp.pointer, resp.size);
        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_sync(messenger& m, const messenger::header& h) {
        auto promise = std::make_shared<awaitable_promise<void>>(m_ioc);
        boost::asio::post(m_pool, [&, this]() {
            try {
                m_data_store.sync();
                promise->set();

            } catch (const std::exception& e) {
                promise->set_exception(std::current_exception());
            }
        });

        LOG_DEBUG() << "handle_sync: waiting for promise";
        co_await promise->get();
        co_await m.send(SUCCESS, {});
        LOG_DEBUG() << "handle_sync: return";
    }

    coro<void> handle_get_used(messenger& m, const messenger::header&) {
        const auto used = m_data_store.get_used_space();
        co_await m.send_uint128_t(SUCCESS, used);
    }

    uh::cluster::data_store m_data_store;
    boost::asio::thread_pool m_pool;
    boost::asio::io_context& m_ioc;
};

} // end namespace uh::cluster

#endif // CORE_DATA_STORE_SERVICE_HANDLER_H
