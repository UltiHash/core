#include "fragment_set.h"
#include "deduplicator/config.h"

namespace uh::cluster {
fragment_set::fragment_set(const std::filesystem::path& set_log_path,
                           size_t capacity, global_data_view& storage,
                           bool enable_replay)
    : m_storage(storage),
      m_set_log(set_log_path),
      m_lfu(capacity, [this](const auto& key, const auto& element) {
          fragment_set_log::log_entry entry{set_operation::REMOVE, key};
          m_set_log.append(entry);
          if (element->m_hint_count > 0) {
              element->m_state = COLD;
              m_colds.emplace_front(element);
          } else {
              m_set.erase(element);
          }
          std::erase_if(m_colds, [](const auto& item) {
              return item->m_hint_count == 0;
          });
      }) {
    if (enable_replay)
        m_set_log.replay(m_set, m_storage);
}

fragment_set::response fragment_set::find(std::string_view data) {
    auto prefix = data.substr(0, std::min(PREFIX_SIZE, data.size()));
    fragment_set_element f{data, std::string(prefix), m_storage};

    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto res = m_set.lower_bound(f);

    response resp;
    if (res->m_state != COLD) {
        resp.hint.emplace(res);
    }

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
                          const std::string_view& data,
                          const std::optional<hint_type>& hint) {

    auto prefix = data.substr(0, std::min(PREFIX_SIZE, data.size()));
    fragment_set_element f{data, pointer, std::string(prefix), m_storage};
    fragment_set_log::log_entry entry{set_operation::INSERT, f.pointer(),
                                      f.size(), f.prefix()};
    m_set_log.append(entry);

    metric<metric_type::deduplicator_set_fragment_counter>::increase(1);
    metric<metric_type::deduplicator_set_fragment_size_counter, byte>::increase(
        data.size());

    if (hint) {
        auto res = m_set.emplace_hint(hint->m_hint, std::move(f));
        if (res->pointer() == pointer) {
            m_lfu.put_non_existing(pointer, std::move(res));
        }
    } else {
        auto res = m_set.emplace(std::move(f));
        if (res.second) {
            m_lfu.put_non_existing(pointer, std::move(res.first));
        }
    }
}

void fragment_set::mark_deduplication(const fragment& frag) {
    m_lfu.use(frag.pointer);
}

void fragment_set::flush() { m_set_log.flush(); }

size_t fragment_set::size() { return m_set.size(); }

std::lock_guard<std::shared_mutex> fragment_set::lock() {
    return std::lock_guard<std::shared_mutex>(m_mutex);
}

} // namespace uh::cluster
