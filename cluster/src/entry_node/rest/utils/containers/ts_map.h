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
        void insert(const T& key, const Y& value);
        void clear();

        std::map<T,Y>::iterator find(const T& key);
        Y& operator[] (const T& key);

        typename std::map<T, Y>::iterator begin();
        typename std::map<T, Y>::iterator end();

    };

} // uh::cluster::rest::utils
