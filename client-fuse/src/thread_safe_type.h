//
// Created by masi on 4/14/23.
//

#ifndef CORE_THREAD_SAFE_TYPE_H
#define CORE_THREAD_SAFE_TYPE_H

#include <uhv/meta_data.h>

#include <mutex>


namespace uh::fuse
{

template <typename T>
class thread_safe_type {
public:

    class type_handle
    {

    public:
        ~type_handle() {
            m_ts.unlock();
        }

        T& operator()() {
            return m_t;
        }

        T* operator->()
        {
            return &m_t;
        }

        T& operator*()
        {
            return m_t;
        }

    private:
        friend thread_safe_type;

        type_handle(thread_safe_type& ts, T& t): m_ts(ts), m_t(t) {}

        thread_safe_type &m_ts;
        T& m_t;

    };

    thread_safe_type () = default;
    thread_safe_type (T t): m_t (std::move (t)) {}

    type_handle get() {
        m_mutex.lock();
        return {*this, m_t};
    }

private:
    friend class type_handle;

    void unlock() {
        m_mutex.unlock();
    }

    std::recursive_mutex m_mutex;
    T m_t;
};

using ts_meta_data = thread_safe_type<uhv::meta_data>;
using ts_container = thread_safe_type< std::unordered_map<std::string, ts_meta_data> >;

}

#endif //CORE_THREAD_SAFE_TYPE_H
