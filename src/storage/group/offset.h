#pragma once

#include "impl/prefix.h"

#include <common/etcd/subscriber.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>
#include <storage/group/state.h>

namespace uh::cluster::storage {

class offset_manager {
public:
    using callback_t = subscriber::callback_t;
    offset_manager(etcd_manager& etcd, std::size_t group_id,
                   std::size_t num_storages)
        : m_etcd{etcd},
          m_prefix{get_storage_offset_prefix(group_id)},
          m_num_storages{num_storages} {}

    ~offset_manager() { m_etcd.rm(m_prefix); }

    static void put(etcd_manager& etcd, std::size_t group_id,
                    std::size_t storage_id, std::size_t val) {
        etcd.put(get_storage_offset_prefix(group_id)[storage_id],
                 serialize(val));
    }

    auto summarize_offsets(std::chrono::seconds timeout = 2s) {
        auto start_time = std::chrono::steady_clock::now();

        std::size_t max_offset = 0;
        while (true) {
            auto m_offset_candidates =
                sync_vector_observer<std::optional<std::size_t>>(
                    m_prefix, m_num_storages, std::nullopt);

            for (auto i = 0ul; i < m_num_storages; ++i) {
                LOG_DEBUG() << *m_offset_candidates.get(i);
            }
            reader r("", m_etcd, m_prefix, {m_offset_candidates});
            auto candidates = m_offset_candidates.get();

            bool all_read = [&]() {
                for (auto i = 0ul; i < m_offset_candidates.get().size(); ++i) {
                    if (!m_offset_candidates.get(i).has_value()) {
                        return false;
                    }
                }
                return true;
            }();

            auto current_time = std::chrono::steady_clock::now();
            if (all_read or current_time - start_time > timeout) {
                auto max_offset_it = std::ranges::max_element(
                    candidates,
                    []<typename T>(const T& a, const T& b) { return a < b; });

                max_offset =
                    (max_offset_it != candidates.end() ? **max_offset_it : 0);
                break;
            }
            std::this_thread::yield();
        }

        return max_offset;
    }

private:
    etcd_manager& m_etcd;
    offset_prefix_t m_prefix;
    std::size_t m_num_storages;
};

} // namespace uh::cluster::storage
