#pragma once

#include <mutex>
#include <proxy/cache/cache.h>
#include <queue>

namespace uh::cluster {

template <typename Key, typename Value>
requires detail::has_size<Value> && std::movable<Value>
class deletion_queue {
public:
    using entry = typename cache<Key, Value>::entry;
    void enqueue(std::shared_ptr<entry> e) {
        // TODO: Implement this method
        std::unique_lock lock(m_mutex);
        m_queue.push(e);
        m_current_size += e->value.size();
    }
    void enqueue(std::vector<std::shared_ptr<entry>> ve) {
        std::unique_lock lock(m_mutex);
        for (auto& e : ve) {
            m_queue.push(e);
            m_current_size += e->value.size();
        }
    }
    std::vector<entry> dequeue(std::size_t size = 0) {
        std::unique_lock lock(m_mutex);
        std::vector<entry> ret;
        while (!m_queue.empty() && (size == 0 || m_current_size > size)) {
            auto e = m_queue.front();
            m_queue.pop();
            m_current_size -= e->value.size();
            ret.push_back(std::move(*e));
        }
        return ret;
    }

private:
    std::mutex m_mutex;
    std::queue<std::shared_ptr<entry>> m_queue;
    std::size_t m_current_size = 0;
};

} // namespace uh::cluster
