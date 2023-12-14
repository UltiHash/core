//
// Created by masi on 8/29/23.
//

#ifndef CORE_DEDUPE_NODE_HANDLER_H
#define CORE_DEDUPE_NODE_HANDLER_H

#include <utility>

#include "common/utils/common.h"
#include "common/utils/cluster_config.h"
#include "common/utils/protocol_handler.h"
#include "dedupe_set.h"

namespace uh::cluster {

class deduplicator_handler: public protocol_handler {

public:

    deduplicator_handler (dedupe_config dedupe_conf, global_data_view& storage, std::shared_ptr <boost::asio::thread_pool> dedupe_workers):
        protocol_handler (dedupe_conf.server_conf),
        m_dedupe_conf (std::move(dedupe_conf)),
        m_fragment_set (m_dedupe_conf.set_log_path, storage),
        m_storage (storage),
        m_dedupe_workers (std::move (dedupe_workers)),
        m_counters (add_counter_family("uh_dd_requests", "number of requests handled by the deduplication node")),
        m_reqs_dedupe (m_counters.Add({{"type", "DEDUPE_REQ"}}))
        {
            if (m_dedupe_conf.min_fragment_size > m_storage.l1_cache_sample_size()) {
                throw std::invalid_argument ("L1 cache sample size should not be smaller than the min fragment size!");
            }
            init();
            boost::asio::post (*m_dedupe_workers, [&] () {m_fragment_set.load();});
        }

    coro <void> handle (messenger m) override {

        for (;;) {
            std::optional<error> err;

            try {
                const auto message_header = co_await m.recv_header();
                switch (message_header.type) {
                    case DEDUPE_REQ:
                        m_reqs_dedupe.Increment();
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

        unique_buffer<char> data(h.size);
        m.register_read_buffer(data);
        co_await m.recv_buffers(h);

        const auto pieces = std::min (m_dedupe_conf.data_node_connection_count,
                                      static_cast <int> (std::ceil (static_cast <double> (data.size()) /
                                      static_cast <double> (m_dedupe_conf.dedupe_worker_minimum_data_size))));
        size_t piece_size = std::ceil (static_cast <double> (data.size()) / pieces);
        std::vector <dedupe_response> responses (pieces);
        std::atomic <int> resp = 0;
        boost::asio::steady_timer waiter (*m_storage.get_executor(), boost::asio::steady_timer::clock_type::duration::max ());

        for (int i = 0; i < pieces; ++i) {
            boost::asio::post(*m_dedupe_workers, [&responses, i, &data, piece_size, &resp, pieces, &waiter, this] () {
                const auto data_piece = data.get_str_view().substr(i * piece_size, std::min (piece_size, data.size() - i*piece_size));
                responses[i] = deduplicate (data_piece);
                auto count = resp++;
                if (count == pieces - 1)
                    waiter.expires_at(boost::asio::steady_timer::time_point::min());
            });
        }

        co_await waiter.async_wait(as_tuple(boost::asio::use_awaitable));

        for (int i = 1; i < pieces; i++) {
            responses[0].addr.append_address(responses[i].addr);
            responses[0].effective_size += responses[i].effective_size;
        }
        co_await m.send_dedupe_response(DEDUPE_RESP, responses[0]);

    }

    dedupe_response deduplicate (std::string_view data) {
        dedupe_response result {.addr = address {}};
        auto integration_data = data;

        auto check_dedupe = [&] (const dedupe_set::fragment_element& frag) {
            auto frag_data = m_storage.read_l1_cache(frag.pointer, frag.size);
            bool l1 = true;
            if (frag_data.data() == nullptr) {
                l1 = false;
                frag_data = dedupe_set::load_fragment(frag, m_storage);
            }
            auto common_prefix = largest_common_prefix(integration_data, frag_data.get_str_view());
            if (common_prefix >= m_dedupe_conf.min_fragment_size) {
                if (common_prefix == m_storage.l1_cache_sample_size() and l1) {
                    frag_data = dedupe_set::load_fragment(frag, m_storage);
                    common_prefix += largest_common_prefix(integration_data.substr(common_prefix), frag_data.get_str_view().substr(common_prefix));
                }
                result.addr.push_fragment(fragment{frag.pointer, common_prefix});
                integration_data = integration_data.substr(common_prefix);
                return true;
            }
            return false;
        };

        while (!integration_data.empty()) {
            const auto f = m_fragment_set.find (integration_data);

            if (f.low.has_value()) {
                if (check_dedupe (f.low->get())) {
                    continue;
                }
            }
            if (f.high.has_value()) {
                if (check_dedupe (f.high->get())) {
                    continue;
                }
            }

            const auto frag_size = std::min (integration_data.size (), m_dedupe_conf.max_fragment_size);
            const auto addr = store_data(integration_data.substr(0, frag_size));
            m_fragment_set.insert({addr.pointers[0], addr.pointers[1]},
                                  integration_data.substr(0, addr.sizes.front()), f.hint);
            result.addr.append_address(addr);
            result.effective_size += frag_size;
            integration_data = integration_data.substr(frag_size);
        }

        m_storage.sync(result.addr);
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

    address store_data (const std::string_view& frag) {
        return m_storage.write(frag);
    }

    dedupe_config m_dedupe_conf;
    dedupe_set m_fragment_set;
    global_data_view& m_storage;
    std::shared_ptr <boost::asio::thread_pool> m_dedupe_workers;
    prometheus::Family<prometheus::Counter>& m_counters;
    prometheus::Counter& m_reqs_dedupe;
};

} // end namespace uh::cluster

#endif //CORE_DEDUPE_NODE_HANDLER_H
