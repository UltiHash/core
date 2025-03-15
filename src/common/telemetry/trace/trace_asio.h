#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <opentelemetry/context/context.h>
#include <opentelemetry/trace/context.h>
#include <opentelemetry/trace/provider.h>
#include <source_location>

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

class _string {
    const char* str;

public:
    explicit _string(const char* s)
        : str(s) {}

    const char* c_str() const { return str; }
};

class trace_span {
public:
    inline static bool enable = false;
    inline static std::string tracer_name = "default-tracer";
    inline static std::string tracer_version = "0.1.0";

    trace_span(const std::source_location& location) noexcept
        : m_file_name{location.file_name()},
          m_line{location.line()},
          m_function_name{location.function_name()},
          m_location{location} {}

    ~trace_span() {
        if (is_started())
            m_data->End();
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
            std::cerr << "Span is not started: [" << m_location.file_name()
                      << ":" << m_location.line() << "] " << coroutine_name()
                      << "\n";
        }
        return opentelemetry::context::Context();
    }

    void
    process_execution_sequence(std::function<void(std::string_view)> process) {
        auto parent = m_parent;
        while (parent) {
            process(parent->m_location.function_name());
            parent = parent->m_parent;
        }
    }
    // assert(is_started() && "Span is not started");

    static std::string
    trace_id(const trace_api::SpanContext& context) noexcept {
        std::array<char, 2 * trace_api::TraceId::kSize> print_buffer{};
        context.trace_id().ToLowerBase16(print_buffer);
        return std::string(print_buffer.data(), print_buffer.size());
    }

private:
    template <typename, typename> friend class boost::asio::traced_awaitable;
    template <typename, typename>
    friend class boost::asio::detail::traced_awaitable_frame;
    friend boost::asio::this_coro::span_t;
    friend boost::asio::this_coro::context_t;

    // Debugging facilities
    trace_span* m_parent{nullptr};
    _string m_file_name;
    uint32_t m_line;
    _string m_function_name;

    std::source_location m_location;

    opentelemetry::nostd::shared_ptr<trace_api::Span> m_data;

    static auto root_context() noexcept {
        opentelemetry::context::Context context;
        return context.SetValue(trace_api::kIsRootSpanKey, true);
    }

    // Debugging facilities
    void set_parent(trace_span* parent) { m_parent = parent; }

    void start_span(opentelemetry::context::Context context) noexcept {
        if (enable) {
            auto tracer = trace_api::Provider::GetTracerProvider()->GetTracer(
                tracer_name, tracer_version);
            m_data = tracer->StartSpan(coroutine_name(), {.parent = context});
            decorate_span();
        }
    }

    bool is_started() noexcept { return m_data != nullptr; }

    std::string coroutine_name() noexcept {
        return extract_function_name(m_location.function_name());
    }

    void decorate_span() noexcept {
        m_data->SetAttribute("function name", m_location.function_name());
        m_data->SetAttribute("file", m_location.file_name());
        m_data->SetAttribute("line", std::to_string(m_location.line()));
    }
    static std::string
    extract_function_name(std::string_view function_signature) noexcept {
        auto last_space =
            function_signature.rfind(' ', function_signature.find('('));
        if (last_space == std::string_view::npos) {
            last_space = 0;
        } else {
            last_space++;
        }

        auto name_start = last_space;

        auto paren = function_signature.find('(', name_start);
        if (paren == std::string_view::npos) {
            return std::string(function_signature);
        }

        return std::string(
            function_signature.substr(name_start, paren - name_start));
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

namespace detail {
template <typename T, typename Executor>
class traced_awaitable_frame : public awaitable_frame<T, Executor> {
public:
    using awaitable_frame<T, Executor>::awaitable_frame;

    traced_awaitable<T, Executor> get_return_object() noexcept {
        return traced_awaitable<T, Executor>(
            awaitable_frame<T, Executor>::get_return_object(), this);
    }

    auto initial_suspend(const std::source_location& location =
                             std::source_location::current()) noexcept {
        m_span.emplace(location);
        return awaitable_frame<T, Executor>::initial_suspend();
    }

    auto final_suspend() noexcept {
        m_span.reset();
        return awaitable_frame<T, Executor>::final_suspend();
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

    trace_span* span() noexcept { return m_span ? &*m_span : nullptr; }

private:
    std::optional<trace_span> m_span;
};

// void specialization
template <typename Executor>
class traced_awaitable_frame<void, Executor>
    : public awaitable_frame<void, Executor> {
public:
    using awaitable_frame<void, Executor>::awaitable_frame;

    traced_awaitable<void, Executor> get_return_object() noexcept {
        return traced_awaitable<void, Executor>(
            awaitable_frame<void, Executor>::get_return_object(), this);
    }

    auto initial_suspend(const std::source_location& location =
                             std::source_location::current()) noexcept {
        m_span.emplace(location);
        return awaitable_frame<void, Executor>::initial_suspend();
    }

    auto final_suspend() noexcept {
        m_span.reset();
        return awaitable_frame<void, Executor>::final_suspend();
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

    trace_span* span() noexcept { return m_span ? &*m_span : nullptr; }

private:
    std::optional<trace_span> m_span;
};

} // namespace detail

} // namespace asio
} // namespace boost

namespace std {
template <typename T, typename Executor, typename... Args>
struct coroutine_traits<boost::asio::traced_awaitable<T, Executor>, Args...> {
    typedef boost::asio::detail::traced_awaitable_frame<T, Executor>
        promise_type;
};
} // namespace std
