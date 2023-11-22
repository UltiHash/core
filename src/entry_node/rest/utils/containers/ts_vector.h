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

        explicit ts_vector(std::string value)
        {
            m_container.emplace_back(std::move(value));
        }

        void push_back(const T& value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_container.push_back(value);
        }

        void remove(const T& value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = std::find(m_container.begin(), m_container.end(), value);
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