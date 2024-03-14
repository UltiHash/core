#include "fragment_set.h"

namespace uh::cluster {
fragment_set::fragment_set(const std::filesystem::path& set_log_path,
                           global_data_view& storage)
    : m_storage(storage),
      m_set_log(set_log_path) {
    m_set_log.replay(m_set, m_storage);
}

fragment_set::response fragment_set::find(std::string_view data) {
    fragment_set_element f{data, m_storage};
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const auto res = m_set.lower_bound(f);

    response resp{.hint = res};
    if (res != m_set.cend()) [[likely]] {
        resp.high.emplace(*res);
    }
    if (res != m_set.cbegin()) [[likely]] {
        resp.low.emplace(*std::prev(res));
    }

    return resp;
}

void fragment_set::insert(
    const uint128_t& pointer, const std::string_view& data,
    const std::set<fragment_set_element>::const_iterator& hint) {
    fragment_set_element f{data, pointer, m_storage};
    m_set_log.append(
        {set_operation::INSERT, f.pointer(), f.size(), f.prefix()});
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    m_set.emplace_hint(hint, std::move(f));
}
} // namespace uh::cluster