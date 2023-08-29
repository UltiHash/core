//
// Created by masi on 8/29/23.
//

#ifndef CORE_DEDUPE_NODE_HANDLER_H
#define CORE_DEDUPE_NODE_HANDLER_H

#include <utility>

#include "common/protocol_handler.h"

namespace uh::cluster {

class dedupe_node_handler: public protocol_handler {

public:

    dedupe_node_handler (dedupe_config dedupe_conf, global_data& storage):
        m_dedupe_conf (std::move(dedupe_conf)),
        m_fragment_set (m_dedupe_conf.set_conf, storage),
        m_storage (storage) {}

    void handle (messenger m) override {
        auto message = m.recv();
        switch (message.first) {
            case DEDUPE_REQ:
                handle_dedupe (m, std::move (message.second));
                break;
            default:
                throw std::invalid_argument ("Invalid message type!");
        }
    }

private:

    void handle_dedupe (messenger& m, ospan<char> data) {
        const auto result = deduplicate ({data.data.get(), data.size});
        // TODO: send effective size
        m.send (DEDUPE_RESP, {reinterpret_cast <const char*> (result.second.data()), result.second.size()});
    }

    std::pair <std::size_t, address> deduplicate (std::string_view data) {

        std::pair <std::size_t, address> result;
        auto integration_data = data;

        while (!integration_data.empty()) {
            const auto f = m_fragment_set.find(integration_data);
            if (f.match) {
                result.second.emplace_back(wide_span{f.match->data_offset, integration_data.size()});
                integration_data = integration_data.substr(integration_data.size());
                continue;
            }

            const std::string_view lower_data_str {f.lower->data.data.get(), f.lower->data.size};
            const auto lower_common_prefix = largest_common_prefix (integration_data, lower_data_str);

            if (lower_common_prefix == integration_data.size()) {
                result.second.emplace_back(wide_span {f.lower->data_offset, integration_data.size()});
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
                const auto addr = store_data(integration_data.substr(0, size));
                m_fragment_set.add_pointer (integration_data.substr(0, addr.front().size), addr.front().pointer, f.index);

                result.second.insert(result.second.cend(), addr.cbegin(), addr.cend());
                result.first += size;
                integration_data = integration_data.substr(size);
                continue;
            }
            else if (max_common_prefix == integration_data.size()) {
                result.second.emplace_back(wide_span {max_data_offset, integration_data.size()});
                integration_data = integration_data.substr(integration_data.size());
                continue;
            }
            else {
                result.second.emplace_back (wide_span {max_data_offset, max_common_prefix});
                integration_data = integration_data.substr(max_common_prefix, integration_data.size() - max_common_prefix);
                continue;
            }

        }

        m_storage.sync(result.second);
        return result;
    }

    static size_t largest_common_prefix(const std::string_view &str1, const std::string_view &str2) noexcept {
        size_t i = 0;
        const auto min_size = std::min (str1.size(), str2.size());
        while (i < min_size and str1[i] == str2[i]) {
            i++;
        }
        return i;
    }

    address store_data(const std::string_view& frag) {
        return m_storage.write(frag);
    }

    dedupe_config m_dedupe_conf;
    dedupe::paged_redblack_tree <dedupe::set_full_comparator> m_fragment_set;
    global_data& m_storage;


};

} // end namespace uh::cluster

#endif //CORE_DEDUPE_NODE_HANDLER_H
