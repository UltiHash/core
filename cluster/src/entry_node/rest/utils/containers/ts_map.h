#pragma once

#include <map>
#include <mutex>

namespace uh::cluster::rest::utils
{

    template <typename T, typename Y>
    class ts_map
    {
    private:
        std::map<T,Y> m_container;
        std::mutex m_mutex;

    public:
        void insert(const T& key, const Y& value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_container.insert(key, value);
        }
        void clear()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_container.clear();
        }

        typename std::map<T,Y>::iterator find(const T& key)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_container.find(key);
        }

        Y& operator[] (const T& key)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_container[key];
        }

        typename std::map<T, Y>::iterator begin()
        {
            return m_container.begin();
        }
        typename std::map<T, Y>::iterator end()
        {
            return m_container.end();
        }

    };

} // uh::cluster::rest::utils
