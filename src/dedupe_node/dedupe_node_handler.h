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


namespace uh::cluster {

class dedupe_node_handler: public protocol_handler {

public:

    dedupe_node_handler (dedupe_config dedupe_conf, global_data_view& storage):
        m_dedupe_conf (std::move(dedupe_conf)),
        m_fragment_set (m_dedupe_conf.set_conf, storage),
        m_storage (storage) {}

    coro <void> handle (messenger m) override {

        for (;;) {
            const auto message_header = co_await m.recv_header();
            switch (message_header.type) {
                case DEDUPE_REQ:
                    co_await handle_dedupe(m, message_header);
                    break;
                default:
                    throw std::invalid_argument("Invalid message type!");
            }
        }
    }

private:

    coro <void> handle_dedupe (messenger& m, const messenger::header& h) {

        ospan<char> data (h.size);
        m.register_read_buffer(data);
        co_await m.recv_buffers(h);

        const auto result = co_await deduplicate ({data.data.get(), data.size});
        co_await m.send_dedupe_response(DEDUPE_RESP, result);
    }

    coro <dedupe_response> deduplicate (std::string_view data) {

        dedupe_response result {.addr = address {}};
        auto integration_data = data;
        dedupe_write_cache cache(integration_data, m_storage, m_dedupe_conf);
        while (!integration_data.empty()) {
            const auto f = co_await m_fragment_set.find(integration_data);
            if (f.match) {
                result.addr.push_fragment (fragment {f.match->data_offset, integration_data.size()});
                integration_data = integration_data.substr(integration_data.size());
                continue;
            }

            const std::string_view lower_data_str {f.lower->data.data.get(), f.lower->data.size};
            const auto lower_common_prefix = largest_common_prefix (integration_data, lower_data_str);

            if (lower_common_prefix == integration_data.size()) {
                result.addr.push_fragment (fragment {f.lower->data_offset, integration_data.size()});
                integration_data = integration_data.substr(integration_data.size());
                continue;
            }

            const std::string_view upper_data_str {f.upper->data.data.get(), f.upper->data.size};
            const auto upper_common_prefix = largest_common_prefix (integration_data, upper_data_str);
            auto max_common_prefix = upper_common_prefix;
            auto max_data_offset = f.upper->data_offset;
            if (max_common_prefix < lower_common_prefix) {
                max_common_prefix = lower_common_prefix;
                max_data_offset = f.lower->data_offset;
            }

            if (max_common_prefix < m_dedupe_conf.min_fragment_size or integration_data.size() - max_common_prefix < m_dedupe_conf.min_fragment_size) {

                const auto size = std::min (integration_data.size(), m_dedupe_conf.max_fragment_size);
                const auto addr = co_await store_data(integration_data.substr(0, size));
                //const auto addr = std::move(co_await cache.write(integration_data.substr(0, size)));
                m_fragment_set.add_pointer (integration_data.substr(0, addr.sizes.front()), {addr.pointers[0], addr.pointers[1]}, f.index);

                result.addr.append_address(addr);
                result.effective_size += size;
                integration_data = integration_data.substr(size);
                continue;
            }
            else if (max_common_prefix == integration_data.size()) {
                result.addr.push_fragment(fragment {max_data_offset, integration_data.size()});
                integration_data = integration_data.substr(integration_data.size());
                continue;
            }
            else {
                result.addr.push_fragment (fragment {max_data_offset, max_common_prefix});
                integration_data = integration_data.substr(max_common_prefix, integration_data.size() - max_common_prefix);
                continue;
            }

        }

        //co_await cache.flush();
        co_await m_storage.sync(result.addr);
        co_return std::move (result);
    }

    static size_t largest_common_prefix(const std::string_view &str1, const std::string_view &str2) noexcept {
        size_t i = 0;
        const auto min_size = std::min (str1.size(), str2.size());
        while (i < min_size and str1[i] == str2[i]) {
            i++;
        }
        return i;
    }

    coro <address> store_data(const std::string_view& frag) {
        co_return std::move (co_await m_storage.write(frag));
    }

    dedupe_config m_dedupe_conf;
    dedupe::paged_redblack_tree <dedupe::set_full_comparator> m_fragment_set;
    global_data_view& m_storage;
    std::mutex m_mutex;

};

} // end namespace uh::cluster

#endif //CORE_DEDUPE_NODE_HANDLER_H
