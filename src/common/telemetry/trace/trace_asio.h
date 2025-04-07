#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <opentelemetry/context/context.h>
#include <opentelemetry/trace/context.h>
#include <opentelemetry/trace/provider.h>

namespace boost {
namespace asio {

template <typename, typename> class traced_awaitable;

namespace this_coro {
struct span_t {
    constexpr span_t() {}
};

inline constexpr span_t span;

struct context_t {
    constexpr context_t() {}
};

inline constexpr context_t context;
} // namespace this_coro

namespace detail {
template <typename, typename> class traced_awaitable_frame;
}

class trace_span {
public:
    inline static bool enable = false;
    inline static std::string tracer_name = "default-tracer";
    inline static std::string tracer_version = "0.1.0";

    trace_span(){};

    ~trace_span() {
        if (is_started())
            m_data->End();
    }

    void set_location(const source_location& location) noexcept {
        m_location = location;
    }

    void set_name(std::string_view name) noexcept {
        if (enable) {
            if (is_started()) {
                m_data->UpdateName(opentelemetry::nostd::string_view(
                    name.begin(), name.size()));
            }
        }
    }

    template <typename Value>
    void set_attribute(std::string_view key, Value value) noexcept {
        if (enable) {
            if (is_started()) {
                m_data->SetAttribute(
                    opentelemetry::nostd::string_view(key.begin(), key.size()),
                    value);
            }
        }
    }

    template <class U,
              std::enable_if_t<opentelemetry::common::detail::
                                   is_key_value_iterable<U>::value>* = nullptr>
    void add_event(std::string_view name, const U& attributes) noexcept {
        if (enable) {
            if (is_started()) {
                m_data->AddEvent(opentelemetry::nostd::string_view(name.begin(),
                                                                   name.size()),
                                 attributes);
            }
        }
    }

    void
    add_event(std::string_view name,
              const std::initializer_list<std::pair<std::string, std::string>>&
                  attributes) noexcept {
        std::vector<std::pair<std::string, std::string>> attrs(attributes);
        add_event(name, attrs);
    }

    void add_event(std::string_view name) noexcept {
        if (enable) {
            if (is_started()) {
                m_data->AddEvent(opentelemetry::nostd::string_view(
                    name.begin(), name.size()));
            }
        }
    }

    auto context() noexcept {
        if (enable) {
            if (is_started()) {
                opentelemetry::context::Context context;
                return opentelemetry::trace::SetSpan(context, m_data);
            }
        }
        return opentelemetry::context::Context();
    }

    bool is_started() noexcept { return m_data != nullptr; }

    // For Debugging
    void iterate_call_stack(std::function<void(source_location)> process) {
        auto parent = m_parent;
        while (parent) {
            process(parent->m_location);
            parent = parent->m_parent;
        }
    }

    // For Debugging
    static std::string
    trace_id(const trace_api::SpanContext& context) noexcept {
        std::array<char, 2 * trace_api::TraceId::kSize> print_buffer{};
        context.trace_id().ToLowerBase16(print_buffer);
        return std::string(print_buffer.data(), print_buffer.size());
    }

    // For Debugging
    static bool check_context(const opentelemetry::context::Context& context) {
        auto span = opentelemetry::trace::GetSpan(context);
        auto span_context = span->GetContext();
        bool is_valid = span_context.IsValid();

        return is_valid;
    }

private:
    template <typename, typename> friend class boost::asio::traced_awaitable;
    template <typename, typename>
    friend class boost::asio::detail::traced_awaitable_frame;
    friend boost::asio::this_coro::span_t;
    friend boost::asio::this_coro::context_t;

    // For Debugging
    trace_span* m_parent{nullptr};

    source_location m_location;

    opentelemetry::nostd::shared_ptr<trace_api::Span> m_data;

    static auto root_context() noexcept {
        opentelemetry::context::Context context;
        return context.SetValue(trace_api::kIsRootSpanKey, true);
    }

    // For Debugging
    void set_parent(trace_span* parent) { m_parent = parent; }

    void start_span(opentelemetry::context::Context context) noexcept {
        if (enable) {
            auto tracer = trace_api::Provider::GetTracerProvider()->GetTracer(
                tracer_name, tracer_version);
            m_data = tracer->StartSpan(m_location.function_name(),
                                       {.parent = context});
            decorate_span();
        }
    }

    void decorate_span() noexcept {
        m_data->SetAttribute("function name", m_location.function_name());
        m_data->SetAttribute("file", m_location.file_name());
        m_data->SetAttribute("line", std::to_string(m_location.line()));
    }
};

template <typename T, typename Executor = any_io_executor>
class BOOST_ASIO_NODISCARD traced_awaitable : public awaitable<T, Executor> {
public:
    using awaitable<T, Executor>::awaitable;
    explicit traced_awaitable(
        awaitable<T, Executor>&& other,
        detail::traced_awaitable_frame<T, Executor>* frame)
        : awaitable<T, Executor>(std::move(other)),
          m_frame{frame} {}

    template <class U>
    void await_suspend(
        detail::coroutine_handle<detail::traced_awaitable_frame<U, Executor>>
            h) {
        auto& parent_promise = h.promise();

        auto parent_span = parent_promise.span();

        if (m_frame != nullptr) {
            auto current_span = m_frame->span();
            current_span->set_parent(parent_span);
            if (!current_span->is_started() && parent_span->is_started()) {
                current_span->start_span(parent_span->context());
            }
        }

        awaitable<T, Executor>::await_suspend(
            detail::coroutine_handle<detail::awaitable_frame<U, Executor>>::
                from_promise(parent_promise));
    }
    detail::traced_awaitable_frame<T, Executor>* get_coroutine_frame() {
        return m_frame;
    }

    auto& continue_trace(opentelemetry::context::Context parent_context) & {
        if (m_frame != nullptr) {
            auto current_span = m_frame->span();
            if (!current_span->is_started()) {
                current_span->start_span(parent_context);
            }
        }
        return *this;
    }

    auto&& continue_trace(opentelemetry::context::Context parent_context) && {
        return std::move(this->continue_trace(parent_context));
    }

    auto& start_trace() & { return continue_trace(trace_span::root_context()); }

    auto&& start_trace() && { return std::move(this->start_trace()); }

    auto& set_name(std::string_view name) & noexcept {
        if (m_frame != nullptr) {
            m_frame->span()->set_name(name);
        }
        return *this;
    }

    auto&& set_name(std::string_view name) && noexcept {
        return std::move(this->set_name(name));
    }

    template <typename Value>
    auto& set_attribute(std::string_view key, Value value) & noexcept {
        if (m_frame != nullptr) {
            m_frame->span()->set_attribute(key, value);
        }
        return *this;
    }

    template <typename Value>
    auto&& set_attribute(std::string_view key, Value value) && noexcept {
        return std::move(this->set_attribute(key, value));
    }

    template <class U,
              std::enable_if_t<opentelemetry::common::detail::
                                   is_key_value_iterable<U>::value>* = nullptr>
    auto& add_event(std::string_view name, const U& attributes) & noexcept {
        if (m_frame != nullptr) {
            m_frame->span()->add_event(name, attributes);
        }
        return *this;
    }

    template <class U,
              std::enable_if_t<opentelemetry::common::detail::
                                   is_key_value_iterable<U>::value>* = nullptr>
    auto&& add_event(std::string_view name, const U& attributes) && noexcept {
        return std::move(this->add_event(name, attributes));
    }

    auto&
    add_event(std::string_view name,
              const std::initializer_list<std::pair<std::string, std::string>>&
                  attributes) & noexcept {
        if (m_frame != nullptr) {
            m_frame->span()->add_event(name, attributes);
        }
        return *this;
    }

    auto&&
    add_event(std::string_view name,
              const std::initializer_list<std::pair<std::string, std::string>>&
                  attributes) && noexcept {
        return std::move(this->add_event(name, attributes));
    }

    auto& add_event(std::string_view name) & noexcept {
        if (m_frame != nullptr) {
            m_frame->span()->add_event(name);
        }
        return *this;
    }

    auto&& add_event(std::string_view name) && noexcept {
        return std::move(this->add_event(name));
    }

private:
    detail::traced_awaitable_frame<T, Executor>* m_frame{nullptr};
};

#define CURRENT_LOCATION                                                       \
    ::boost::source_location(__builtin_FILE(), __builtin_LINE(),               \
                             __builtin_FUNCTION())

namespace detail {
template <typename T, typename Executor>
class traced_awaitable_frame : public awaitable_frame<T, Executor> {
public:
    using awaitable_frame<T, Executor>::awaitable_frame;

    auto initial_suspend(
        const source_location& location = CURRENT_LOCATION) noexcept {
        m_span.set_location(location);
        return awaitable_frame<T, Executor>::initial_suspend();
    }

    traced_awaitable<T, Executor> get_return_object() noexcept {
        return traced_awaitable<T, Executor>(
            awaitable_frame<T, Executor>::get_return_object(), this);
    }

    using awaitable_frame_base<Executor>::await_transform;

    template <typename U>
    auto await_transform(traced_awaitable<U, Executor> a) const {
        return traced_awaitable<U, Executor>(
            awaitable_frame_base<Executor>::await_transform(
                std::move(static_cast<awaitable<U, Executor>&>(a))),
            a.get_coroutine_frame());
    }

    template <typename U> auto await_transform(awaitable<U, Executor> a) const {
        return traced_awaitable<U, Executor>(
            awaitable_frame_base<Executor>::await_transform(std::move(a)),
            nullptr);
    }

    auto await_transform(this_coro::span_t) noexcept {
        struct result {
            traced_awaitable_frame* this_;

            bool await_ready() const noexcept { return true; }

            void await_suspend(coroutine_handle<void>) noexcept {}

            auto await_resume() const noexcept { return this_->span(); }
        };

        return result{this};
    }

    auto await_transform(this_coro::context_t) noexcept {
        struct result {
            traced_awaitable_frame* this_;

            bool await_ready() const noexcept { return true; }

            void await_suspend(coroutine_handle<void>) noexcept {}

            auto await_resume() const noexcept {
                return this_->span()->context();
            }
        };

        return result{this};
    }

    trace_span* span() noexcept { return &m_span; }

private:
    trace_span m_span;
};

// void specialization
template <typename Executor>
class traced_awaitable_frame<void, Executor>
    : public awaitable_frame<void, Executor> {
public:
    using awaitable_frame<void, Executor>::awaitable_frame;

    auto initial_suspend(
        const source_location& location = CURRENT_LOCATION) noexcept {
        m_span.set_location(location);
        return awaitable_frame<void, Executor>::initial_suspend();
    }

    traced_awaitable<void, Executor> get_return_object() noexcept {
        return traced_awaitable<void, Executor>(
            awaitable_frame<void, Executor>::get_return_object(), this);
    }

    using awaitable_frame_base<Executor>::await_transform;

    template <typename U>
    auto await_transform(traced_awaitable<U, Executor> a) const {
        return traced_awaitable<U, Executor>(
            awaitable_frame_base<Executor>::await_transform(
                std::move(static_cast<awaitable<U, Executor>&>(a))),
            a.get_coroutine_frame());
    }

    template <typename U> auto await_transform(awaitable<U, Executor> a) const {
        return traced_awaitable<U, Executor>(
            awaitable_frame_base<Executor>::await_transform(std::move(a)),
            nullptr);
    }

    auto await_transform(this_coro::span_t) noexcept {
        struct result {
            traced_awaitable_frame* this_;

            bool await_ready() const noexcept { return true; }

            void await_suspend(coroutine_handle<void>) noexcept {}

            auto await_resume() const noexcept { return this_->span(); }
        };

        return result{this};
    }

    auto await_transform(this_coro::context_t) noexcept {
        struct result {
            traced_awaitable_frame* this_;

            bool await_ready() const noexcept { return true; }

            void await_suspend(coroutine_handle<void>) noexcept {}

            auto await_resume() const noexcept {
                return this_->span()->context();
            }
        };

        return result{this};
    }

    trace_span* span() noexcept { return &m_span; }

private:
    trace_span m_span;
};

} // namespace detail

namespace detail {

template <typename T> struct traced_awaitable_signature;

template <typename T, typename Executor>
struct traced_awaitable_signature<traced_awaitable<T, Executor>> {
    typedef void type(std::exception_ptr, T);
};

template <typename Executor>
struct traced_awaitable_signature<traced_awaitable<void, Executor>> {
    typedef void type(std::exception_ptr);
};

} // namespace detail

template <
    typename Executor, typename F,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(typename detail::traced_awaitable_signature<
                                    result_of_t<F()>>::type) CompletionToken>
inline BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    typename detail::traced_awaitable_signature<result_of_t<F()>>::type)
    co_spawn(const Executor& ex, F&& f, CompletionToken&& token,
             constraint_t<is_executor<Executor>::value ||
                          execution::is_executor<Executor>::value> = 0) {
    return async_initiate<
        CompletionToken,
        typename detail::traced_awaitable_signature<result_of_t<F()>>::type>(
        detail::initiate_co_spawn<typename result_of_t<F()>::executor_type>(ex),
        token, std::forward<F>(f));
}

template <
    typename ExecutionContext, typename F,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(typename detail::traced_awaitable_signature<
                                    result_of_t<F()>>::type) CompletionToken>
inline BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    typename detail::traced_awaitable_signature<result_of_t<F()>>::type)
    co_spawn(ExecutionContext& ctx, F&& f, CompletionToken&& token,
             constraint_t<is_convertible<ExecutionContext&,
                                         execution_context&>::value> = 0) {
    return (co_spawn)(ctx.get_executor(), std::forward<F>(f),
                      std::forward<CompletionToken>(token));
}

} // namespace asio
} // namespace boost

namespace std {
template <typename T, typename Executor, typename... Args>
struct coroutine_traits<boost::asio::traced_awaitable<T, Executor>, Args...> {
    typedef boost::asio::detail::traced_awaitable_frame<T, Executor>
        promise_type;
};
} // namespace std
