#pragma once

#include <vector>
#include <mutex>

namespace uh::cluster::rest::utils
{
    template <typename T>
    class ts_vector
    {
    private:
        std::vector<T> m_container{};
        std::mutex m_mutex{};

    public:
        ts_vector() = default;

        void push_back(const T& value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_container.push_back(value);
        }

        void clear()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_container.clear();
        }

        typename std::vector<T>::iterator begin()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_container.begin();
        }

        typename std::vector<T>::iterator end()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_container.end();
        }

        size_t size()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_container.size();
        }

        T operator[](size_t index)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_container[index];
        }
    };
} // uh::cluster::rest::utils