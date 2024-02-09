#ifndef UH_CLUSTER_LFU_CACHE_H
#define UH_CLUSTER_LFU_CACHE_H

#include <unordered_map>
#include <optional>
#include <list>
#include <map>
#include <memory>

namespace uh::cluster {

template <typename Key, typename Value>
class lfu_cache {

    struct key_order_data {
        int frequency;
        const Key key;
    };

    struct key_map_data {
        std::list <key_order_data>::iterator order_pos;
        Value value;
    };

    std::list <key_order_data> m_order;

    std::map <Key, key_map_data> m_key_values;
    const size_t m_capacity;

public:
    explicit lfu_cache(size_t capacity): m_capacity {capacity} {
    }

    void put (const Key& key, Value&& val) {

        if (const auto it = m_key_values.find (key); it != m_key_values.cend()) {
            update_order (it->second.order_pos);
        }
        else {
            auto pos = m_order.begin();

            while (pos != m_order.end () and pos->frequency == 1) {
                pos = std::next (pos);
            }

            pos = m_order.emplace(pos, 1, key);
            m_key_values.emplace_hint(it, key, key_map_data {pos, std::move (val)});

            if (m_key_values.size() > m_capacity) {
                const auto rk = m_order.front().key;
                m_key_values.erase (rk);
                m_order.pop_front();
            }
        }
    }

    std::optional <std::reference_wrapper <Value>> get (const Key& key) {
        if (const auto it = m_key_values.find (key); it != m_key_values.cend()) {
            update_order (it->second.order_pos);
            return it->second.value;
        }
        return std::nullopt;
    }

private:

    void update_order (std::list <key_order_data>::iterator& pos) {
        pos->frequency ++;

        const auto freq = pos->frequency;
        auto nx = std::next(pos);
        while (nx != m_order.end () and nx->frequency <= freq) {
            nx ++;
        }
        m_order.splice (nx, m_order, pos);

    }

};
} // end namespace uh::cluster

#endif //UH_CLUSTER_LFU_CACHE_H
