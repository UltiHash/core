
#ifndef UH_CLUSTER_MAP_INDEX_H
#define UH_CLUSTER_MAP_INDEX_H

#include <map>
#include <stdexcept>

namespace uh::cluster {

template <typename K, typename V> struct map_index {

    void add(const K& k, const V& v) {
        auto itr = m_v_to_k.find(v);
        if (itr != m_v_to_k.cend()) {
            m_k_to_v.erase(itr->second);
            itr->second = m_k_to_v.emplace(k, v);
        } else {
            m_v_to_k.emplace_hint(itr, v, m_k_to_v.emplace(k, v));
        }
    }

    auto remove(const V& v) {
        auto itr = m_v_to_k.find(v);
        if (itr == m_v_to_k.cend()) {
            throw std::runtime_error("non-existing value in map index");
        }
        auto r = m_k_to_v.erase(itr->second);
        m_v_to_k.erase(itr);
        return r;
    }

    [[nodiscard]] auto max() const { return std::prev(m_k_to_v.cend()); }

    [[nodiscard]] auto end() const noexcept { return m_k_to_v.cend(); }

    [[nodiscard]] auto begin() const noexcept { return m_k_to_v.cbegin(); }

    [[nodiscard]] optref<const V> at(const K& k) const {
        if (auto it = m_k_to_v.find(k); it != m_k_to_v.cend()) {
            return it->second;
        }
        return std::nullopt;
    }

    [[nodiscard]] const K& get_key(const V& v) const {
        auto itr = m_v_to_k.find(v);
        if (itr != m_v_to_k.cend()) {
            return itr->second->first;
        }
        throw std::runtime_error("non-existing value in map index");
    }

    [[nodiscard]] inline size_t size() const noexcept {
        return m_k_to_v.size();
    }

private:
    std::multimap<K, V> m_k_to_v;
    std::map<V, decltype(m_k_to_v.cbegin())> m_v_to_k;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_MAP_INDEX_H
