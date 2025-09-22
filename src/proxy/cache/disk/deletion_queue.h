#pragma once

#include <proxy/cache/cache.h>

#include <mutex>
#include <queue>
#include <shared_mutex>

// TODO: See if we can use boost::lockfree::queue here.

namespace uh::cluster::proxy::cache::disk {

template <typename Key, EntryType Entry> class deletion_queue {
public:
    void push(std::shared_ptr<Entry> e) {
        std::unique_lock lock(m_mutex);
        m_queue.push(e);
        m_current_size += e->data_size();
    }

    void push(std::vector<std::shared_ptr<Entry>> ve) {
        std::unique_lock lock(m_mutex);
        for (auto& e : ve) {
            m_queue.push(e);
            m_current_size += e->data_size();
        }
    }

    std::vector<std::shared_ptr<Entry>> pop(std::size_t size) {
        std::unique_lock lock(m_mutex);
        std::vector<std::shared_ptr<Entry>> ret;
        while (!m_queue.empty() && (size != 0)) {
            auto e = m_queue.front();
            m_queue.pop();
            size = (size > e->data_size()) ? size - e->data_size() : 0;
            m_current_size -= e->data_size();
            ret.push_back(std::move(e));
        }
        return ret;
    }

    std::size_t data_size() const {
        std::shared_lock lock(m_mutex);
        return m_current_size;
    }

private:
    mutable std::shared_mutex m_mutex;

    std::queue<std::shared_ptr<Entry>> m_queue;
    std::size_t m_current_size = 0;
};

} // namespace uh::cluster::proxy::cache::disk
