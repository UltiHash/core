#include "fragment_set.h"
#include "deduplicator/config.h"

namespace uh::cluster {

fragment_set::fragment_set(size_t capacity, storage::global::cache& storage)
    : m_storage(storage),
      m_lfu(capacity, std::bind_front(&fragment_set::remove, this)),
      m_lfu_headers(capacity, std::bind_front(&fragment_set::remove, this)) {}

fragment_set::response fragment_set::find(std::string_view data) {
    auto prefix = data.substr(0, std::min(PREFIX_SIZE, data.size()));
    fragment_set_element f{data, std::string(prefix), m_storage};

    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto res = m_set.lower_bound(f);

    response resp;
    resp.hint.emplace(res);

    if (res != m_set.cend()) [[likely]] {
        resp.high.emplace(fragment{res->pointer(), res->size()}, res->prefix());
    }
    if (res != m_set.cbegin()) [[likely]] {
        res--;
        resp.low.emplace(fragment{res->pointer(), res->size()}, res->prefix());
    }
    return resp;
}

void fragment_set::insert(const uint128_t& pointer, std::string_view data,
                          bool header, const std::optional<hint_type>& hint) {
    auto prefix = data.substr(0, std::min(PREFIX_SIZE, data.size()));
    fragment_set_element f{data, pointer, std::string(prefix), m_storage};

    metric<metric_type::deduplicator_set_fragment_counter>::increase(1);
    metric<metric_type::deduplicator_set_fragment_size_counter, byte>::increase(
        data.size());

    if (hint) {
        auto res = m_set.emplace_hint(hint->m_hint, std::move(f));
        if (res->pointer() == pointer) {
            if (header)
                m_lfu_headers.put_non_existing(pointer, res);
            else
                m_lfu.put_non_existing(pointer, res);
        }
    } else {
        auto res = m_set.emplace(std::move(f));
        if (res.second) {
            if (header)
                m_lfu_headers.put_non_existing(pointer, res.first);
            else
                m_lfu.put_non_existing(pointer, res.first);
        }
    }
}

void fragment_set::mark_deduplication(const fragment& frag) {
    m_lfu.use(frag.pointer);
}

size_t fragment_set::size() const { return m_set.size(); }

std::lock_guard<std::shared_mutex> fragment_set::lock() {
    return std::lock_guard<std::shared_mutex>(m_mutex);
}

void fragment_set::remove(
    const std::set<fragment_set_element>::const_iterator& itr) {
    if (itr->m_hint_count == 0) {
        m_set.erase(itr);
    }
}

void fragment_set::erase(const fragment& set_element, bool header) {
    std::lock_guard<std::shared_mutex> guard(m_mutex);
    if (header) {
        m_lfu_headers.erase(set_element.pointer);
    } else {
        m_lfu.erase(set_element.pointer);
    }
}

} // namespace uh::cluster
