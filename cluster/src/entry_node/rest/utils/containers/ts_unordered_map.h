#pragma once

#include <unordered_map>
#include <mutex>

namespace uh::cluster::rest::utils
{

    template <typename T, typename Y>
    class ts_unordered_map
    {
    private:
        std::unordered_map<T,Y> m_container {};
        std::mutex m_mutex {};

    public:
        void emplace(T key, Y value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_container.emplace(std::move(key), std::move(value));
        }

        void remove(const T& key)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_container.erase(key);
        }

        std::unordered_map<T,Y>::iterator find(const T& key)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_container.find(key);
        }

        std::unordered_map<T,Y>::iterator begin()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_container.begin();
        }

        std::unordered_map<T,Y>::iterator end()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_container.end();
        }

    };

} // uh::cluster::rest::utils
