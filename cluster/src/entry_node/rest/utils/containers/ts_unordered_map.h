#pragma once

#include <unordered_map>
#include <mutex>

namespace uh::cluster::rest::utils
{

    template <typename T, typename Y>
    class ts_unordered_map
    {
    private:
        std::unordered_map<T,Y> m_container;
        std::mutex m_mutex;

    public:
        void ts_insert(const T& key, const Y& value);
        void ts_remove(const T& key);

    };

} // uh::cluster::rest::utils
