#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <opentelemetry/trace/provider.h>

namespace boost {
namespace asio {

namespace detail {
template <typename, typename> class traced_awaitable_frame;
class otel_span;
} // namespace detail

template <typename T, typename Executor = any_io_executor>
class BOOST_ASIO_NODISCARD traced_awaitable : public awaitable<T, Executor> {
public:
    using awaitable<T, Executor>::awaitable;
    explicit traced_awaitable(
        awaitable<T, Executor>&& other,
        detail::traced_awaitable_frame<T, Executor>* frame)
        : awaitable<T, Executor>(std::move(other)),
          m_frame{frame} {
        // std::cout << "traced_awaitable' constructor\n";
        // std::cout << frame << std::endl;
    }

    template <class U>
    void await_suspend(
        detail::coroutine_handle<detail::traced_awaitable_frame<U, Executor>>
            h) {
        auto& parent_promise = h.promise();
        // std::cout << parent_promise.get_coroutine_name() << " suspended\n";
        // std::cout << "await_suspend\n";
        if (m_frame) {
            auto parent_span = parent_promise.get_span();
            if (parent_span) {
                // std::cout << "start span with parent context\n";
                m_frame->start(parent_span->GetContext());
            } else {
                // std::cout << "start first span\n";
                m_frame->start();
            }
        } else {
            // std::cerr << "m_frame is not initialized\n";
        }

        awaitable<T, Executor>::await_suspend(
            detail::coroutine_handle<detail::awaitable_frame<U, Executor>>::
                from_promise(parent_promise));
    }
    detail::traced_awaitable_frame<T, Executor>* get_coroutine_frame() {
        return m_frame;
    }

private:
    detail::traced_awaitable_frame<T, Executor>* m_frame;
};

namespace detail {

class otel_span {
public:
    void set_source_location(const std::source_location& location) {
        m_location = location;
    }

    void start(const opentelemetry::trace::SpanContext& parent_context) {
        // std::cout << "start span for " << get_coroutine_name() << std::endl;
        // std::cout << "- using parent context(e.g. parent_span_id: "
        //           << get_trace_id(parent_context) << ")\n";
        auto tracer =
            opentelemetry::trace::Provider::GetTracerProvider()->GetTracer(
                "my-app-tracer");
        m_span =
            tracer->StartSpan(get_coroutine_name(), {.parent = parent_context});

        decorate_span();
    }

    void start() {
        // std::cout << "start span for " << get_coroutine_name() << std::endl;
        auto tracer =
            opentelemetry::trace::Provider::GetTracerProvider()->GetTracer(
                "my-app-tracer");
        m_span = tracer->StartSpan(get_coroutine_name());
        decorate_span();
    }

    void stop() {
        // std::cout << "stop span" << std::endl;
        if (m_span)
            m_span->End();
    }

    void set_span(
        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span) {
        m_span = span;
    }
    auto get_span() { return m_span; }
    auto get_source_location() { return m_location; }
    // void print_span_trace_id()

    std::string get_coroutine_name() {
        return extract_function_name(m_location.function_name());
    }

    static std::string
    get_trace_id(const opentelemetry::trace::SpanContext& context) {
        std::array<char, 2 * opentelemetry::trace::TraceId::kSize>
            print_buffer{};
        context.trace_id().ToLowerBase16(print_buffer);
        return std::string(print_buffer.data(), print_buffer.size());
    }

private:
    std::source_location m_location;
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> m_span;
    void decorate_span() {
        // std::cout << "trace_id: " << get_trace_id(m_span->GetContext())
        //           << std::endl;

        m_span->SetAttribute("file", m_location.file_name());
        m_span->SetAttribute("line", std::to_string(m_location.line()));

        // span->SetStatus(trace::StatusCode::kError, "Exception occurred");
    }
    static std::string
    extract_function_name(std::string_view function_signature) {
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

} // namespace detail

struct set_trace_parent_span {
    set_trace_parent_span(
        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>
            parent_span)
        : m_parent_span{parent_span} {}

    bool await_ready() const noexcept { return false; }

    template <typename U, typename Executor = any_io_executor>
    void await_suspend(
        std::coroutine_handle<detail::traced_awaitable_frame<U, Executor>> h) {
        auto& parent_promise = h.promise();
        parent_promise.set_span(m_parent_span);
        h.resume();
    }

    void await_resume() const noexcept {}

private:
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> m_parent_span;
};

struct manual_start_trace_span {
    manual_start_trace_span() {}

    bool await_ready() const noexcept { return false; }

    template <typename U, typename Executor = any_io_executor>
    void await_suspend(
        std::coroutine_handle<detail::traced_awaitable_frame<U, Executor>> h) {
        if (h.promise().get_span()) {
            std::cerr << "span is already set: you should remove co_await "
                         "get_trace_span()\n";
            exit(0);
        }
        // std::cout << "start first span for " <<
        // h.promise().get_coroutine_name()
        //           << "on get_trace_span\n";
        h.promise().start();
        // std::cout << h.promise().get_span() << std::endl;
        m_frame = (detail::otel_span*)&h.promise();
        h.resume();
    }

    auto await_resume() const noexcept { return m_frame->get_span(); }

private:
    detail::otel_span* m_frame;
};

struct get_trace_span {
    get_trace_span() {}

    bool await_ready() const noexcept { return false; }

    template <typename U, typename Executor = any_io_executor>
    void await_suspend(
        std::coroutine_handle<detail::traced_awaitable_frame<U, Executor>> h) {
        m_frame = (detail::otel_span*)&h.promise();
        h.resume();
    }

    auto await_resume() const noexcept { return m_frame->get_span(); }

private:
    detail::otel_span* m_frame;
};

namespace detail {
// Primary template
template <typename T, typename Executor>
class traced_awaitable_frame : public awaitable_frame<T, Executor>,
                               public otel_span {
public:
    // TRACED_AWAITABLE_FRAME_COMMON_METHODS(T, Executor)
    using awaitable_frame<T, Executor>::awaitable_frame;
    traced_awaitable<T, Executor> get_return_object() {
        // std::cout << "get_return_object\n";
        // std::cout << this << std::endl;
        return traced_awaitable<T, Executor>(
            awaitable_frame<T, Executor>::get_return_object(), this);
    }
    auto initial_suspend(const std::source_location& location =
                             std::source_location::current()) noexcept {
        otel_span::set_source_location(location);
        return awaitable_frame<T, Executor>::initial_suspend();
    }
    auto final_suspend() noexcept {
        otel_span::stop();
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
    auto await_transform(set_trace_parent_span&& a) const {
        return std::forward<set_trace_parent_span>(a);
    }
    auto await_transform(get_trace_span&& a) const {
        return std::forward<get_trace_span>(a);
    }
    auto await_transform(manual_start_trace_span&& a) const {
        return std::forward<manual_start_trace_span>(a);
    }
};

// void specialization
template <typename Executor>
class traced_awaitable_frame<void, Executor>
    : public awaitable_frame<void, Executor>, public otel_span {
public:
public:
    using awaitable_frame<void, Executor>::awaitable_frame;
    traced_awaitable<void, Executor> get_return_object() {
        // std::cout << "get_return_object\n";
        // std::cout << this << std::endl;
        return traced_awaitable<void, Executor>(
            awaitable_frame<void, Executor>::get_return_object(), this);
    }
    auto initial_suspend(const std::source_location& location =
                             std::source_location::current()) noexcept {
        otel_span::set_source_location(location);
        return awaitable_frame<void, Executor>::initial_suspend();
    }
    auto final_suspend() noexcept {
        otel_span::stop();
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
    auto await_transform(set_trace_parent_span&& a) const {
        return std::forward<set_trace_parent_span>(a);
    }
    auto await_transform(get_trace_span&& a) const {
        return std::forward<get_trace_span>(a);
    }
    auto await_transform(manual_start_trace_span&& a) const {
        return std::forward<manual_start_trace_span>(a);
    }
};

// awaitable_thread_entry_point specialization
// template <typename Executor>
// class traced_awaitable_frame<awaitable_thread_entry_point, Executor>
//     : public awaitable_frame<awaitable_thread_entry_point, Executor>,
//       public otel_span {
// public:
//     TRACED_AWAITABLE_FRAME_COMMON_METHODS(awaitable_thread_entry_point,
//                                           Executor)
// };

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
