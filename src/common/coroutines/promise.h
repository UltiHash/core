#ifndef UH_CLUSTER_COROUTINES_PROMISE_H
#define UH_CLUSTER_COROUTINES_PROMISE_H

#include "common/debug/debug.h"
#include "common/types/common_types.h"

#include <boost/asio.hpp>

#include <memory>
#include <mutex>
#include <optional>

namespace uh::cluster {

enum class state_type { promise_created, future_created };

template <typename value>
class shared_state : public std::enable_shared_from_this<shared_state<value>> {
public:
    ~shared_state() { m_value = std::nullopt; }

    void register_handler(auto completion_handler) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_completion) {
            throw std::runtime_error("future::get() called multiple times");
        }

        m_completion = make_completion(std::move(completion_handler));

        if (m_value || m_exception) {
            lock.unlock();
            m_completion->call(this->shared_from_this());
        }
    }

    void set_value(value&& v) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_value || m_exception) {
            throw std::future_error(
                std::future_errc::promise_already_satisfied);
        }

        m_value = std::move(v);

        if (m_completion) {
            lock.unlock();
            m_completion->call(this->shared_from_this());
        }
    }

    void set_exception(std::exception_ptr e) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_value || m_exception) {
            throw std::future_error(
                std::future_errc::promise_already_satisfied);
        }

        m_exception = e;

        if (m_completion) {
            lock.unlock();
            m_completion->call(this->shared_from_this());
        }
    }

    state_type type() const { return m_type; }
    void type(state_type type) { m_type = type; }

private:
    struct _completion_base {
        virtual ~_completion_base() = default;
        virtual void call(std::shared_ptr<shared_state>) = 0;
    };

    template <typename handler> struct completion : _completion_base {
        completion(handler h)
            : m_handler(std::move(h)) {}

        virtual void call(std::shared_ptr<shared_state> s) {
            /* Note: shared_ptr is required to move ownership of the shared
             * state to the executor.
             */
            boost::asio::post(get_associated_executor(m_handler),
                              [s, this]() -> void {
                                  auto exc = s->m_exception.value_or(nullptr);
                                  if (s->m_value) {
                                      m_handler(exc, std::move(*s->m_value));
                                  } else {
                                      /* Note: ASIO's handler prototype require
                                       * that you pass a value. This creates the
                                       * requirement that passed values must be
                                       * default-constructible.
                                       */
                                      m_handler(exc, value{});
                                  }
                              });
        }

        handler m_handler;
    };

    std::unique_ptr<_completion_base> make_completion(auto handler) {
        return std::make_unique<completion<decltype(handler)>>(
            std::move(handler));
    }

    std::unique_ptr<_completion_base> m_completion;

    std::optional<value> m_value;
    std::optional<std::exception_ptr> m_exception;
    std::mutex m_mutex;
    state_type m_type = state_type::promise_created;
};

template <>
class shared_state<void>
    : public std::enable_shared_from_this<shared_state<void>> {
public:
    void register_handler(auto completion_handler) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_completion) {
            throw std::runtime_error("future::get() called multiple times");
        }

        m_completion = make_completion(std::move(completion_handler));

        if (m_value || m_exception) {
            lock.unlock();
            m_completion->call(this->shared_from_this());
        }
    }

    void set_value() {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_value || m_exception) {
            throw std::future_error(
                std::future_errc::promise_already_satisfied);
        }

        m_value = true;

        if (m_completion) {
            lock.unlock();
            m_completion->call(this->shared_from_this());
        }
    }

    void set_exception(std::exception_ptr e) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_value || m_exception) {
            throw std::future_error(
                std::future_errc::promise_already_satisfied);
        }

        m_exception = e;

        if (m_completion) {
            lock.unlock();
            m_completion->call(this->shared_from_this());
        }
    }

    state_type type() const { return m_type; }
    void type(state_type type) { m_type = type; }

private:
    struct _completion_base {
        virtual ~_completion_base() = default;
        virtual void call(std::shared_ptr<shared_state>) = 0;
    };

    template <typename handler> struct completion : _completion_base {
        completion(handler h)
            : m_handler(std::move(h)) {}

        virtual void call(std::shared_ptr<shared_state> s) {
            /* Note: shared_ptr is required to move ownership of the shared
             * state to the executor.
             */
            boost::asio::post(get_associated_executor(m_handler),
                              [s, this]() -> void {
                                  m_handler(s->m_exception.value_or(nullptr));
                              });
        }

        handler m_handler;
    };

    std::unique_ptr<_completion_base> make_completion(auto handler) {
        return std::make_unique<completion<decltype(handler)>>(
            std::move(handler));
    }

    std::unique_ptr<_completion_base> m_completion;
    bool m_value = false;
    std::optional<std::exception_ptr> m_exception;
    std::mutex m_mutex;
    state_type m_type = state_type::promise_created;
};

template <typename value> class promise;

template <typename value> class future {
public:
    future() noexcept
        : m_shared_state() {}

    future(future&& other) noexcept
        : m_shared_state(std::move(other.m_shared_state)) {}

    future(const future&) = delete;

    future& operator=(future&& other) noexcept {
        m_shared_state = std::move(other.m_shared_state);
        return *this;
    }

    future& operator=(const future&) = delete;

    template <typename completion_token = boost::asio::use_awaitable_t<>>
    auto get(completion_token&& token = {}) {
        if (!m_shared_state) {
            throw std::future_error(std::future_errc::no_state);
        }

        auto state = m_shared_state;
        m_shared_state.reset();

        return boost::asio::async_initiate<completion_token,
                                           void(std::exception_ptr, value)>(
            [state](auto completion_handler) {
                state->register_handler(std::move(completion_handler));
            },
            token);
    }

    bool valid() const noexcept { return m_shared_state.get() != nullptr; }

private:
    friend class promise<value>;

    future(std::shared_ptr<shared_state<value>> state)
        : m_shared_state(state) {}

    std::shared_ptr<shared_state<value>> m_shared_state;
};

template <> class future<void> {
public:
    future() noexcept
        : m_shared_state() {}

    future(future&& other) noexcept
        : m_shared_state(std::move(other.m_shared_state)) {}

    future(const future&) = delete;

    future& operator=(future&& other) noexcept {
        m_shared_state = std::move(other.m_shared_state);
        return *this;
    }

    future& operator=(const future&) = delete;

    template <typename completion_token = boost::asio::use_awaitable_t<>>
    auto get(completion_token&& token = {}) {
        if (!m_shared_state) {
            throw std::future_error(std::future_errc::no_state);
        }

        auto state = m_shared_state;
        m_shared_state.reset();

        return boost::asio::async_initiate<completion_token,
                                           void(std::exception_ptr)>(
            [state](auto completion_handler) {
                state->register_handler(std::move(completion_handler));
            },
            token);
    }

    bool valid() const noexcept { return m_shared_state.get() != nullptr; }

private:
    friend class promise<void>;

    future(std::shared_ptr<shared_state<void>> state)
        : m_shared_state(state) {}

    std::shared_ptr<shared_state<void>> m_shared_state;
};

template <typename value> class promise {
public:
    promise()
        : m_shared_state(std::make_shared<shared_state<value>>()) {}

    promise(promise&& other) noexcept
        : m_shared_state(std::move(other.m_shared_state)) {}

    promise(const promise&) = delete;

    promise& operator=(promise&& other) noexcept {
        m_shared_state = std::move(other.m_shared_state);
        return *this;
    }

    promise& operator=(const promise&) = delete;

    void swap(promise& other) noexcept {
        std::swap(other.m_shared_state, m_shared_state);
    }

    future<value> get_future() {
        if (!m_shared_state) {
            throw std::future_error(std::future_errc::no_state);
        }

        if (m_shared_state->type() != state_type::promise_created) {
            throw std::future_error(std::future_errc::future_already_retrieved);
        }

        m_shared_state->type(state_type::future_created);
        return future<value>(m_shared_state);
    }

    void set_value(value v) {
        if (!m_shared_state) {
            throw std::future_error(std::future_errc::no_state);
        }

        m_shared_state->set_value(std::move(v));
    }

    void set_exception(std::exception_ptr p) {
        if (!m_shared_state) {
            throw std::future_error(std::future_errc::no_state);
        }

        m_shared_state->set_exception(p);
    }

private:
    std::shared_ptr<shared_state<value>> m_shared_state;
};

template <> class promise<void> {
public:
    promise()
        : m_shared_state(std::make_shared<shared_state<void>>()) {}

    promise(promise&& other) noexcept
        : m_shared_state(std::move(other.m_shared_state)) {}

    promise(const promise&) = delete;

    promise& operator=(promise&& other) noexcept {
        m_shared_state = std::move(other.m_shared_state);
        return *this;
    }

    promise& operator=(const promise&) = delete;

    void swap(promise& other) noexcept {
        std::swap(other.m_shared_state, m_shared_state);
    }

    future<void> get_future() {
        if (!m_shared_state) {
            throw std::future_error(std::future_errc::no_state);
        }

        if (m_shared_state->type() != state_type::promise_created) {
            throw std::future_error(std::future_errc::future_already_retrieved);
        }

        m_shared_state->type(state_type::future_created);
        return future<void>(m_shared_state);
    }

    void set_value() {
        if (!m_shared_state) {
            throw std::future_error(std::future_errc::no_state);
        }

        m_shared_state->set_value();
    }

    void set_exception(std::exception_ptr p) {
        if (!m_shared_state) {
            throw std::future_error(std::future_errc::no_state);
        }

        m_shared_state->set_exception(p);
    }

private:
    friend class future<void>;

    std::shared_ptr<shared_state<void>> m_shared_state;
};

/**
 * Use as a completion token in cojunction with boost async operations.
 *
 * Usage:
 *    promise<std::size_t> p;
 *    auto f = p.get_future();
 *    ioc.async_read(..., ..., use_promise(std::move(p)));
 *    ...
 *    auto result = f.get();
 */
template <typename result>
requires(!std::is_same_v<result, void>)
auto use_promise(promise<result>&& p) {
    return [p = std::move(p)](const boost::system::error_code& e,
                              result r) mutable -> void {
        if (e.failed()) {
            try {
                throw std::runtime_error(e.to_string());
            } catch (const std::exception& e) {
                p.set_exception(std::current_exception());
            }
        } else {
            p.set_value(std::move(r));
        }
    };
}

template <typename result>
requires std::is_same_v<result, void>
auto use_promise(promise<void>&& p) {
    return
        [p = std::move(p)](const boost::system::error_code& e) mutable -> void {
            if (e.failed()) {
                try {
                    throw std::runtime_error(e.to_string());
                } catch (const std::exception& e) {
                    p.set_exception(std::current_exception());
                }
            } else {
                p.set_value();
            }
        };
}

template <typename result>
requires(!std::is_same_v<result, void>)
auto use_promise_cospawn(promise<result>&& p) {
    return [p = std::move(p)](std::exception_ptr e, result r) mutable -> void {
        if (e) {
            p.set_exception(e);
        } else {
            p.set_value(std::move(r));
        }
    };
}

template <typename result>
requires(std::is_same_v<result, void>)
auto use_promise_cospawn(promise<result>&& p) {
    return [p = std::move(p)](std::exception_ptr e) mutable -> void {
        if (e) {
            p.set_exception(e);
        } else {
            p.set_value();
        }
    };
}
} // namespace uh::cluster

#endif
