#include "ts_unordered_map.h"

namespace uh::cluster::rest::utils
{

    template<typename T, typename Y>
    void ts_unordered_map<T,Y>::ts_insert(const T& key, const Y& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_container.insert(key, value);
    }

    template<typename T, typename Y>
    void ts_unordered_map<T,Y>::ts_remove(const T& key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_container.erase(key);
    }

} // uh::cluster::rest::utils