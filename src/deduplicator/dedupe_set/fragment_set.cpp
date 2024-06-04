#include "fragment_set.h"
#include "deduplicator/config.h"

namespace uh::cluster {
fragment_set::fragment_set(const std::filesystem::path& set_log_path,
                           size_t capacity, global_data_view& storage,
                           bool enable_replay)
    : m_storage(storage),
      m_set_log(set_log_path),
      m_lfu(capacity, [this](const auto& key, const auto loc) {
          for (auto [itr, itr_end] = m_hints.equal_range(key); itr != itr_end;
               ++itr)
              itr->second.reset();
          fragment_set_log::log_entry entry{set_operation::REMOVE, key};
          m_set_log.append(entry);
          m_set.erase(loc);
      }) {
    if (enable_replay)
        m_set_log.replay(m_set, m_storage);
}

fragment_set::response fragment_set::find(std::string_view data) {
    auto prefix = data.substr(0, std::min(PREFIX_SIZE, data.size()));
    fragment_set_element f{data, std::string(prefix), m_storage};

    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto res = m_set.lower_bound(f);

    std::unique_lock<std::mutex> hint_lock(m_insert_hint_mutex);
    response resp{.hint = m_hints.emplace(res->pointer(), res)};
    hint_lock.unlock();

    if (res != m_set.cend()) [[likely]] {
        resp.high.emplace(fragment{res->pointer(), res->size()}, res->prefix());
    }
    if (res != m_set.cbegin()) [[likely]] {
        res--;
        resp.low.emplace(fragment{res->pointer(), res->size()}, res->prefix());
    }

    return resp;
}

void fragment_set::insert(const uint128_t& pointer,
                          const std::string_view& data, const hint_type& hint) {

    auto prefix = data.substr(0, std::min(PREFIX_SIZE, data.size()));
    fragment_set_element f{data, pointer, std::string(prefix), m_storage};
    fragment_set_log::log_entry entry{set_operation::INSERT, f.pointer(),
                                      f.size(), f.prefix()};
    m_set_log.append(entry);

    metric<metric_type::deduplicator_set_fragment_counter>::increase(1);
    metric<metric_type::deduplicator_set_fragment_size_counter, byte>::increase(
        data.size());

    std::lock_guard<std::shared_mutex> lock(m_mutex);

    if (hint->second) {
        auto res = m_set.emplace_hint(*hint->second, std::move(f));
        if (res->pointer() == pointer) {
            m_lfu.put_non_existing(pointer, std::move(res));
        }
    } else {
        auto res = m_set.emplace(std::move(f));
        if (res.second) {
            m_lfu.put_non_existing(pointer, std::move(res.first));
        }
    }
    m_hints.erase(hint);
}

void fragment_set::mark_deduplication(const fragment& frag,
                                      const hint_type& hint) {
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    m_hints.erase(hint);
    m_lfu.use(frag.pointer);
}

void fragment_set::flush() { m_set_log.flush(); }

size_t fragment_set::size() { return m_set.size(); }

} // namespace uh::cluster
