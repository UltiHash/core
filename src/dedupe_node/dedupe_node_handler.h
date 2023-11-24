//
// Created by masi on 8/29/23.
//

#ifndef CORE_DEDUPE_NODE_HANDLER_H
#define CORE_DEDUPE_NODE_HANDLER_H

#include <utility>

#include "common/common.h"
#include "common/cluster_config.h"
#include "common/protocol_handler.h"
#include "dedupe_write_cache.h"
#include "dedupe_set.h"
#include "paged_redblack_tree.h"
namespace uh::cluster {

class dedupe_node_handler: public protocol_handler {

public:

    dedupe_node_handler (dedupe_config dedupe_conf, global_data_view& storage, std::shared_ptr <boost::asio::thread_pool> dedupe_workers):
        m_dedupe_conf (std::move(dedupe_conf)),
        m_fragment_set (storage),
        m_storage (storage),
        m_dedupe_workers (std::move (dedupe_workers))
        {}

    coro <void> handle (messenger m) override {

        for (;;) {
            std::optional<error> err;

            try {
                const auto message_header = co_await m.recv_header();
                switch (message_header.type) {
                    case DEDUPE_REQ:
                        co_await handle_dedupe(m, message_header);
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
                co_await m.send_error (*err);
            }
        }
    }

private:

    coro <void> handle_dedupe (messenger& m, const messenger::header& h) {

        if (h.size == 0) [[unlikely]] {
            throw std::length_error ("Empty data sent do the dedupe node");
        }


        ospan<char> data(h.size);
        m.register_read_buffer(data);
        co_await m.recv_buffers(h);

        dedupe_response resp;
        boost::asio::steady_timer waiter (*m_storage.get_executor(), boost::asio::steady_timer::clock_type::duration::max ());
        boost::asio::post(*m_dedupe_workers, [&] () {
            resp = deduplicate (std::move (data));
            waiter.expires_at(boost::asio::steady_timer::time_point::min());
        });
        co_await waiter.async_wait(as_tuple(boost::asio::use_awaitable));

        co_await m.send_dedupe_response(DEDUPE_RESP, resp);

    }

    dedupe_response deduplicate (ospan<char> data) {
        dedupe_response result {.addr = address {}};
        auto integration_data = data.get_str_view();

        while (!integration_data.empty()) {
            const auto f = m_fragment_set.find (integration_data);

            size_t common_prefix = 0;
            uint128_t dedupe_pointer;
            if (f.up.has_value()) {
                const auto upper = dedupe_set::load_fragment(f.up->get(), m_storage);
                common_prefix = largest_common_prefix (integration_data, upper.get_str_view());
                dedupe_pointer = f.up->get().pointer;
            }
            const auto frag_size = std::min (data.size, m_dedupe_conf.max_fragment_size);

            if (common_prefix == frag_size) {
                result.addr.push_fragment (fragment {dedupe_pointer, common_prefix});
                integration_data = integration_data.substr(common_prefix);
                continue;
            }

            if (f.low.has_value()) {
                const auto lower = dedupe_set::load_fragment(f.low->get(), m_storage);
                const auto lower_common_prefix = largest_common_prefix (integration_data, lower.get_str_view());
                if (lower_common_prefix > common_prefix) {
                    common_prefix = lower_common_prefix;
                    dedupe_pointer = f.low->get().pointer;
                }
            }
            if (common_prefix >= m_dedupe_conf.min_fragment_size) {
                result.addr.push_fragment (fragment {dedupe_pointer, common_prefix});
                integration_data = integration_data.substr(common_prefix);
                continue;
            }

            auto addr_fut = boost::asio::co_spawn(*m_storage.get_executor(), store_data (integration_data.substr(0, frag_size)), boost::asio::use_future);
            const auto addr = std::move (addr_fut.get());
            m_fragment_set.insert({addr.pointers[0], addr.pointers[1]}, integration_data.substr(0, addr.sizes.front()), f.hint);
            result.addr.append_address(addr);
            result.effective_size += frag_size;
            integration_data = integration_data.substr(frag_size);

        }

        const auto sync_fut = boost::asio::co_spawn (*m_storage.get_executor(), m_storage.sync(result.addr), boost::asio::use_future);
        sync_fut.wait();
        return result;
    }

    static size_t largest_common_prefix(const std::string_view &str1, const std::string_view &str2) noexcept {
        if (str1.size() <= str2.size()) {
            return std::distance(str1.cbegin(), std::mismatch(str1.cbegin(), str1.cend(), str2.cbegin()).first);
        }
        else {
            return std::distance(str2.cbegin(), std::mismatch(str2.cbegin(), str2.cend(), str1.cbegin()).first);
        }
    }

    coro <address> store_data(const std::string_view& frag) {
        co_return std::move (co_await m_storage.write(frag));
    }

    dedupe_config m_dedupe_conf;
    dedupe_set m_fragment_set;
    global_data_view& m_storage;
    std::shared_ptr <boost::asio::thread_pool> m_dedupe_workers;

};

} // end namespace uh::cluster

#endif //CORE_DEDUPE_NODE_HANDLER_H
