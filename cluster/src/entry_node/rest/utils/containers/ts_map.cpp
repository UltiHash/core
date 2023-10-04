#include "ts_map.h"

namespace uh::cluster::rest::utils {

    template<typename T, typename Y>
    void ts_map<T, Y>::insert(const T &key, const Y &value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_container.insert(key, value);
    }

    template<typename T, typename Y>
    std::map<T, Y>::iterator ts_map<T, Y>::find(const T &key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_container.find(key);
    }

    template<typename T, typename Y>
    Y &ts_map<T, Y>::operator[](const T &key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_container[key];
    }

    template<typename T, typename Y>
    std::map<T, Y>::iterator ts_map<T, Y>::begin()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_container.begin();
    }

    template<typename T, typename Y>
    std::map<T, Y>::iterator ts_map<T, Y>::end()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_container.end();
    }

    template<typename T, typename Y>
    void ts_map<T, Y>::clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_container.clear();
    }

} // uh::cluster::rest::utils
