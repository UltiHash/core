#include "dedupe_set.h"

namespace uh::cluster {
dedupe_set::dedupe_set(const std::filesystem::path& set_log_path,
                       global_data_view& storage)
    : m_storage(storage),
      m_set_log(set_log_path) {}

void dedupe_set::load() { m_set_log.replay(m_set, m_storage, m); }

dedupe_set::response dedupe_set::find(std::string_view data) {
    fragment_element f{data, m_storage};
    std::shared_lock<std::shared_mutex> lock(m);
    const auto res = m_set.lower_bound(f);
    lock.unlock();

    response resp{.hint = res};
    if (res != m_set.cend()) [[likely]] {
        resp.high.emplace(*res);
    }
    if (res != m_set.cbegin()) [[likely]] {
        resp.low.emplace(*std::prev(res));
    }

    return resp;
}

void dedupe_set::insert(
    const uint128_t& pointer, const std::string_view& data,
    const std::set<fragment_element>::const_iterator& hint) {
    fragment_element f{data, pointer, m_storage};
    m_set_log.append(
        {set_operation::INSERT, f.get_pointer(), f.get_size(), f.get_prefix()});
    std::lock_guard<std::shared_mutex> lock(m);
    m_set.emplace_hint(hint, std::move(f));
}
} // namespace uh::cluster