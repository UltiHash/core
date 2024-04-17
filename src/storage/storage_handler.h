#ifndef CORE_DATA_STORE_SERVICE_HANDLER_H
#define CORE_DATA_STORE_SERVICE_HANDLER_H

#include "common/telemetry/metrics.h"
#include "common/utils/common.h"
#include "common/utils/pointer_traits.h"

#include "common/utils/protocol_handler.h"
#include "config.h"
#include "data_store.h"
#include <utility>
#include <execution>

namespace uh::cluster {

class storage_handler : public protocol_handler {
public:
    storage_handler(const data_store_config& config, uint32_t index, uint32_t data_store_count): m_threads(16 * data_store_count) {
        m_data_stores.reserve(data_store_count);
        for (uint32_t i = 0; i < data_store_count; i ++) {
            m_data_stores.emplace_back(std::make_unique<data_store>(config, index, i));
        }
    }


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
                case STORAGE_SYNC_REQ:
                    co_await handle_sync(m, message_header);
                    break;
                case STORAGE_USED_REQ:
                    co_await handle_get_used(m, message_header);
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

        shared_buffer<char> data(h.size);
        m.register_read_buffer(data.data(), data.size());
        co_await m.recv_buffers(h);

        const size_t part = std::ceil ((double) data.size() / m_data_stores.size());

        //std::vector <std::pair <data_store&, std::span <char>>> ds_data;
        //for (size_t i = 0; i < m_data_stores.size(); ++i) {
        //    ds_data.emplace_back(*m_data_stores[i], data.get_span().subspan(i * part, std::min(data.size() - i * part, part)));
        //}
        //auto addresses = co_await m_workers.broadcast_from_io_thread_in_workers([] (auto ds_data) {return ds_data.first.write (ds_data.second);}, ds_data);

        std::vector <std::future <address>> futures;
        futures.reserve (m_data_stores.size());
        address addr;

        for (size_t i = 0; i < m_data_stores.size(); ++i) {
            //addr.append_address(m_data_stores[i]->write(data.get_span().subspan(
            //    i * part, std::min(data.size() - i * part, part))));

            auto p = std::make_shared<std::promise <address>>();
            boost::asio::post (m_threads, [&data, i, this, part, p] () {
                try {
                    p->set_value(
                        m_data_stores[i]->write(data.get_span().subspan(
                            i * part, std::min(data.size() - i * part, part))));
                }
                catch (const std::exception&) {
                    p->set_exception(std::current_exception());
                }
            });
            futures.emplace_back(p->get_future());

        }

        for(auto& f: futures){
            addr.append_address(f.get());
        }
        //for (auto& a: addresses) {
        //    addr.append_address(a);
        //}

        co_await m.send_address(SUCCESS, addr);
    }

    coro<void> handle_read_fragment(messenger& m, const messenger::header& h) {
        const auto resp = co_await m.recv_fragment(h);
        unique_buffer<char> buffer(resp.size);
        const auto size =
            get_data_store(resp.pointer).read(buffer.data(), resp.pointer, resp.size);
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
            if (get_data_store(frag.pointer).read(buffer.data() + offset, frag.pointer,
                                  frag.size) != frag.size) [[unlikely]] {
                throw std::runtime_error(
                    "Could not read the data with the given size");
            }
            offset += frag.size;
        }
        co_await m.send(SUCCESS, {buffer.data(), offset});
    }

    coro<void> handle_sync(messenger& m, const messenger::header& h) {
        std::vector <std::future <void>> futures;
        futures.reserve (m_data_stores.size());
        for (size_t i = 0; i < m_data_stores.size(); ++i) {
            m_data_stores[i]->sync();

            auto p = std::make_shared<std::promise <void>>();
            boost::asio::post (m_threads, [i, this, p] () {
                try {
                    m_data_stores[i]->sync();
                    p->set_value();
                }
                catch (const std::exception&) {
                    p->set_exception(std::current_exception());
                }
            });
            futures.emplace_back(p->get_future());

        }
        for(auto& f: futures){
           f.get();
        }

        //co_await m_workers.broadcast_from_io_thread_in_workers([](const auto& ds){ds->sync(); return SUCCESS;}, m_data_stores);
        co_await m.send(SUCCESS, {});
    }

    coro<void> handle_get_used(messenger& m, const messenger::header&) {
        size_t used = 0;
        for (const auto& ds: m_data_stores) {
            used += ds->get_used_space();
        }
        co_await m.send_uint128_t(SUCCESS, used);
    }

    data_store& get_data_store (const uint128_t& pointer) {
        return *m_data_stores[pointer_traits::get_data_store_id(pointer)];
    }


    std::vector <std::unique_ptr <data_store>> m_data_stores;
    boost::asio::thread_pool m_threads;
};

} // end namespace uh::cluster

#endif // CORE_DATA_STORE_SERVICE_HANDLER_H
