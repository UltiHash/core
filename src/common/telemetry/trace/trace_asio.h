#pragma once

#include <boost/asio.hpp>
#include <chrono>
#include <opentelemetry/context/context.h>
#include <opentelemetry/trace/context.h>
#include <opentelemetry/trace/provider.h>
#include <source_location>

namespace boost {
namespace asio {

namespace detail {
template <typename, typename> class traced_awaitable_frame;
}

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

        if (!parent_span->is_started()) {
            parent_span->start_span();
        }

        if (m_frame != nullptr) {
            auto current_span = m_frame->span();
            if (!current_span->is_started()) {
                current_span->set_parent_span(parent_span);
                current_span->start_span();
            }
        }

        awaitable<T, Executor>::await_suspend(
            detail::coroutine_handle<detail::awaitable_frame<U, Executor>>::
                from_promise(parent_promise));
    }
    detail::traced_awaitable_frame<T, Executor>* get_coroutine_frame() {
        return m_frame;
    }

private:
    detail::traced_awaitable_frame<T, Executor>* m_frame{nullptr};
};

class trace_span {
public:
#ifdef TRACE_SPAN_DEFAULT_ENABLE
    inline static bool enable = true;
#else
    inline static bool enable = false;
#endif

#ifdef OTEL_SPAN_TRACER_NAME
    inline static std::string tracer_name = OTEL_SPAN_TRACER_NAME;
#else
    inline static std::string tracer_name = "my-app-tracer";
#endif

#ifdef OTEL_SPAN_TRACER_VERSION
    inline static std::string tracer_version = OTEL_SPAN_TRACER_VERSION;
#else
    inline static std::string tracer_version = "0.1.0";
#endif

    trace_span(const std::source_location& location) noexcept
        : m_location{location},
          m_start_system_clock{std::chrono::system_clock::now()},
          m_start_steady_clock{std::chrono::steady_clock::now()} {}

    ~trace_span() {
        if (m_data)
            m_data->End();
    }

    void set_name(std::string_view name) noexcept {
        if (m_data)
            m_data->UpdateName(
                opentelemetry::nostd::string_view(name.begin(), name.size()));
    }

    template <typename Value>
    void set_attribute(std::string_view key, Value value) noexcept {
        if (m_data)
            m_data->SetAttribute(key, value);
    }

    auto context() noexcept {
        if (m_data) {
            opentelemetry::context::Context context;
            return opentelemetry::trace::SetSpan(context, m_data);
        } else {
            return opentelemetry::context::Context{};
        }
    }

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

    std::source_location m_location;
    std::chrono::system_clock::time_point m_start_system_clock;
    std::chrono::steady_clock::time_point m_start_steady_clock;

    opentelemetry::nostd::shared_ptr<trace_api::Span> m_data;
    opentelemetry::nostd::variant<opentelemetry::trace::SpanContext,
                                  opentelemetry::context::Context>
        m_parent_context = opentelemetry::trace::SpanContext::GetInvalid();

    void start_span() noexcept {
        if (enable) {
            trace_api::StartSpanOptions options{m_start_system_clock,
                                                m_start_steady_clock};
            auto has_valid_parent = false;
            opentelemetry::nostd::visit(
                [&](const auto& value) {
                    using T = std::decay_t<decltype(value)>;

                    if constexpr (std::is_same_v<
                                      T, opentelemetry::trace::SpanContext>) {
                        has_valid_parent = value.IsValid();
                    } else if constexpr (std::is_same_v<
                                             T,
                                             opentelemetry::context::Context>) {
                        has_valid_parent = true;
                    }
                },
                m_parent_context);

            if (has_valid_parent) {
                options.parent = m_parent_context;
            } else {
                opentelemetry::context::Context root;
                root = root.SetValue(trace_api::kIsRootSpanKey, true);
                options.parent = std::move(root);
            }

            auto tracer = trace_api::Provider::GetTracerProvider()->GetTracer(
                tracer_name, tracer_version);
            std::cout << "function name: " << m_location.function_name()
                      << std::endl;
            std::cout << "coroutine name: " << coroutine_name() << std::endl;
            m_data = tracer->StartSpan(coroutine_name(), options);
            decorate_span();
        }
    }

    bool is_started() noexcept { return m_data != nullptr; }

    void set_parent_span(trace_span* parent_span) noexcept {
        if (m_data) {
            m_parent_context = parent_span->m_data->GetContext();
        }
    }

    void set_context(opentelemetry::context::Context context) noexcept {
        m_parent_context = context;
    }

    std::string coroutine_name() noexcept {
        return extract_function_name(m_location.function_name());
    }

    void decorate_span() noexcept {
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

namespace this_coro {
struct span_t {
    constexpr span_t()
        : m_span{nullptr} {}

    bool await_ready() const noexcept { return false; }

    template <typename U, typename Executor = any_io_executor>
    void await_suspend(
        std::coroutine_handle<detail::traced_awaitable_frame<U, Executor>> h) {
        m_span = h.promise().span();

        if (!m_span->is_started()) {
            m_span->start_span();
        }

        h.resume();
    }

    auto await_resume() const noexcept { return m_span; }

private:
    uh::cluster::trace_span* m_span;
};

inline constexpr span_t span;
} // namespace this_coro

namespace detail {
template <typename T, typename Executor>
class traced_awaitable_frame : public awaitable_frame<T, Executor> {
public:
    using awaitable_frame<T, Executor>::awaitable_frame;

    template <typename... Args> traced_awaitable_frame(Args&&...) noexcept {}

    template <typename... OtherArgs>
    traced_awaitable_frame(const opentelemetry::context::Context& ctx,
                           OtherArgs&&...) noexcept
        : m_context(ctx) {}

    traced_awaitable<T, Executor> get_return_object() noexcept {
        return traced_awaitable<T, Executor>(
            awaitable_frame<T, Executor>::get_return_object(), this);
    }

    auto initial_suspend(const std::source_location& location =
                             std::source_location::current()) noexcept {
        m_span.emplace(location);
        if (m_context)
            m_span->set_context(*m_context);
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

    template <typename U,
              std::enable_if_t<
                  std::is_same_v<std::decay_t<U>, this_coro::span_t>, int> = 0>
    auto await_transform(U&& a) const {
        return std::forward<U>(a);
    }

    uh::cluster::trace_span* span() noexcept {
        return m_span ? &*m_span : nullptr;
    }

private:
    std::optional<uh::cluster::trace_span> m_span;
    std::optional<opentelemetry::context::Context> m_context;
};

// void specialization
template <typename Executor>
class traced_awaitable_frame<void, Executor>
    : public awaitable_frame<void, Executor> {
public:
    using awaitable_frame<void, Executor>::awaitable_frame;

    template <typename... Args> traced_awaitable_frame(Args&&...) noexcept {}

    template <typename... OtherArgs>
    traced_awaitable_frame(const opentelemetry::context::Context& ctx,
                           OtherArgs&&...) noexcept
        : m_context(ctx) {}

    traced_awaitable<void, Executor> get_return_object() noexcept {
        return traced_awaitable<void, Executor>(
            awaitable_frame<void, Executor>::get_return_object(), this);
    }

    auto initial_suspend(const std::source_location& location =
                             std::source_location::current()) noexcept {
        m_span.emplace(location);
        if (m_context)
            m_span->set_context(*m_context);
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

    template <typename U,
              std::enable_if_t<
                  std::is_same_v<std::decay_t<U>, this_coro::span_t>, int> = 0>
    auto await_transform(U&& a) const {
        return std::forward<U>(a);
    }

    uh::cluster::trace_span* span() noexcept {
        return m_span ? &*m_span : nullptr;
    }

private:
    std::optional<uh::cluster::trace_span> m_span;
    std::optional<opentelemetry::context::Context> m_context;
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
