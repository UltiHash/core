#pragma once

#include <proxy/cache/cache.h>

#include <mutex>
#include <queue>

// TODO: See if we can use boost::lockfree::queue here.

namespace uh::cluster::proxy::cache::disk {

template <typename Key, EntryType Entry> class deletion_queue {
public:
    void put(std::shared_ptr<Entry> e) {
        // TODO: Implement this method
        std::unique_lock lock(m_mutex);
        m_queue.push(e);
        m_current_size += e->size();
    }
    void put(std::vector<std::shared_ptr<Entry>> ve) {
        std::unique_lock lock(m_mutex);
        for (auto& e : ve) {
            m_queue.push(e);
            m_current_size += e->size();
        }
    }
    std::vector<std::shared_ptr<Entry>> get(std::size_t size = 0) {
        std::unique_lock lock(m_mutex);
        std::vector<std::shared_ptr<Entry>> ret;
        while (!m_queue.empty() && (size == 0 || m_current_size > size)) {
            auto e = m_queue.front();
            m_queue.pop();
            m_current_size -= e->size();
            ret.push_back(std::move(e));
        }
        return ret;
    }

private:
    std::mutex m_mutex;
    std::queue<std::shared_ptr<Entry>> m_queue;
    std::size_t m_current_size = 0;
};

} // namespace uh::cluster::proxy::cache::disk
