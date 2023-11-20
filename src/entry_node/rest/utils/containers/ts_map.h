#pragma once

#include <map>
#include <mutex>

namespace uh::cluster::rest::utils
{

    template <typename T, typename Y>
    class ts_map
    {
    private:
        std::map<T,Y> m_container {};
        std::mutex m_mutex {};

    public:

        ts_map() = default;

        explicit ts_map(T key)
        {
            m_container.emplace(std::move(key), Y{});
        }

        ts_map(const T& key, const Y& value)
        {
            m_container.insert(key, value);
        }

        void insert(const T& key, const Y& value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_container.insert(key, value);
        }

        void emplace(T key, Y value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_container.emplace(std::move(key), std::move(value));
        }

        void remove(const T& value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_container.find(value);
            if (it != m_container.end())
                m_container.erase(it);
        }

        void clear()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_container.clear();
        }

        bool is_empty()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_container.empty();
        }

        typename std::map<T,Y>::iterator find(const T& key)
        {
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

        size_t size()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_container.size();
        }

    };

} // uh::cluster::rest::utils
