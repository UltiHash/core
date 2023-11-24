//
// Created by massi on 11/22/23.
//

#ifndef UH_CLUSTER_DEDUPLICATOR_H
#define UH_CLUSTER_DEDUPLICATOR_H

#include "common/common_types.h"
#include "common/cluster_config.h"
#include "data_node/data_store.h"
#include "dedupe_mmap_set.h"

namespace uh::cluster {

class deduplicator {

public:

    deduplicator (dedupe_config dedupe_conf, global_data_view& storage): m_dedupe_conf (std::move (dedupe_conf)), m_storage (storage),
                                                                            m_fragment_set(m_dedupe_conf.set_conf, m_storage) {}

    dedupe_response deduplicate (std::string_view data) {

        dedupe_response result {.addr = address {}};
        auto integration_data = data;
        while (!integration_data.empty()) {
            const auto f = m_fragment_set.find(integration_data);
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
                auto addr_fut = boost::asio::co_spawn(*m_storage.get_executor(),
                                                      m_storage.write(integration_data.substr(0, size)),
                                                      boost::asio::use_future);
                const auto addr = std::move(addr_fut.get());
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

        const auto sync_fut = boost::asio::co_spawn (*m_storage.get_executor(), m_storage.sync(result.addr), boost::asio::use_future);
        sync_fut.wait();
        return result;
    }

private:

    static size_t largest_common_prefix(const std::string_view &str1, const std::string_view &str2) noexcept {
        if (str1.size() <= str2.size()) {
            return std::distance(str1.cbegin(), std::mismatch(str1.cbegin(), str1.cend(), str2.cbegin()).first);
        }
        else {
            return std::distance(str2.cbegin(), std::mismatch(str2.cbegin(), str2.cend(), str1.cbegin()).first);
        }
    }

    dedupe_config m_dedupe_conf;
    global_data_view& m_storage;
    dedupe::dedupe_mmap_set m_fragment_set;
};

} // end namespace uh::cluster
#endif //UH_CLUSTER_DEDUPLICATOR_H
