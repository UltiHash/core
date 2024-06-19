#ifndef CORE_COMMON_POOL_H
#define CORE_COMMON_POOL_H

#include "common/coroutines/awaitable_promise.h"
#include "common/debug/debug.h"

#include <list>
#include <memory>

namespace uh::cluster {

template <typename type, typename function>
concept factory = requires(function f) {
    { f() } -> std::same_as<std::unique_ptr<type>>;
};

template <typename resource> class pool {
public:
    class handle {
    public:
        handle(handle&&) = default;

        const resource& get() const { return *m_r; }
        resource& get() { return *m_r; }

        resource* operator->() { return m_r.get(); }
        resource& operator*() { return *m_r; }
        operator resource&() { return m_r; }

        void release() { m_pool.put_back(std::move(m_r)); }

        ~handle() { release(); }

    private:
        friend class pool<resource>;

        handle(std::unique_ptr<resource> r, pool<resource>& pool)
            : m_r(std::move(r)),
              m_pool(pool) {}

        std::unique_ptr<resource> m_r;
        pool<resource>& m_pool;
    };

    template <typename func>
    requires factory<resource, func>
    pool(boost::asio::io_context& ctx, func f, unsigned count)
        : m_ctx(ctx),
          m_mutex(std::make_unique<std::mutex>()) {
        while (count--) {
            m_resources.emplace_back(f());
        }
    }

    coro<handle> get() {
        LOG_CORO_CONTEXT();

        std::shared_ptr<awaitable_promise<handle>> promise;

        {
            std::unique_lock<std::mutex> lk(*m_mutex);

            if (!m_resources.empty()) {
                auto res = std::move(m_resources.front());
                m_resources.pop_front();
                lk.unlock();

                co_return handle(std::move(res), std::ref(*this));
            }

            promise = std::make_shared<awaitable_promise<handle>>(m_ctx);
            m_promises.push_back(promise);
        }

        co_return co_await promise->get();
    }

private:
    friend class handle;

    void put_back(std::unique_ptr<resource> r) {
        if (!r) {
            return;
        }

        LOG_CORO_CONTEXT();

        std::unique_lock<std::mutex> lk(*m_mutex);

        if (!m_promises.empty()) {
            auto promise = m_promises.front();
            m_promises.pop_front();

            promise->set(handle(std::move(r), *this));
            return;
        }

        m_resources.emplace_back(std::move(r));
    }

    boost::asio::io_context& m_ctx;
    std::unique_ptr<std::mutex> m_mutex;
    std::list<std::unique_ptr<resource>> m_resources;
    std::list<std::shared_ptr<awaitable_promise<handle>>> m_promises;
};

} // namespace uh::cluster

#endif
