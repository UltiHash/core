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
        std::pair<typename std::unordered_map<T,Y>::iterator, bool> emplace(T key, Y value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_container.emplace(std::move(key), std::move(value));
        }

        void remove(const T& key)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_container.erase(key);
        }

        Y find(const T& key)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto itr =  m_container.find(key);
            if (itr != m_container.end())
            {
                return itr->second;
            }
            else
            {
                return {};
            }
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
