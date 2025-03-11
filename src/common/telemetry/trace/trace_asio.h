#pragma once

#include <boost/asio.hpp>

#include <common/telemetry/trace/trace_span.h>

#include <common/network/messenger_header.h>

namespace boost {
namespace asio {

namespace detail {
template <typename, typename> class traced_awaitable_frame;
}

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
    trace_span* m_span;
};

inline constexpr span_t span;
} // namespace this_coro

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

static constexpr std::size_t SERIALIZED_SIZE =
    trace_api::TraceId::kSize + trace_api::SpanId::kSize + 2;

inline std::vector<char>
serialize(opentelemetry::nostd::shared_ptr<trace_api::Span> span) {
    std::vector<char> rv(SERIALIZED_SIZE);
    if (!span) {
        return rv;
    }

    auto ctx = span->GetContext();
    std::size_t pos = 0ull;

    auto trace_id = ctx.trace_id().Id();
    std::memcpy(&rv[pos], trace_id.data(), trace_id.size());
    pos += trace_id.size();

    auto span_id = ctx.span_id().Id();
    std::memcpy(&rv[pos], span_id.data(), span_id.size());
    pos += span_id.size();

    rv[pos] = ctx.trace_flags().flags();
    pos += 1;

    rv[pos] = ctx.IsRemote();
    pos += 1;

    return rv;
}

inline opentelemetry::nostd::shared_ptr<trace_api::Span>
deserialize(std::span<const char> buffer) {
    std::span<const uint8_t> uc_buf(
        reinterpret_cast<const uint8_t*>(&buffer[0]), buffer.size());
    std::size_t pos = 0ull;

    std::span<const uint8_t, trace_api::TraceId::kSize> trace_buf(
        &uc_buf[pos], trace_api::TraceId::kSize);
    auto trace_id = trace_api::TraceId(trace_buf);
    pos += trace_api::TraceId::kSize;

    std::span<const uint8_t, trace_api::SpanId::kSize> span_buf(
        &uc_buf[pos], trace_api::SpanId::kSize);
    auto span_id = trace_api::SpanId(span_buf);
    pos += trace_api::SpanId::kSize;

    auto flags = uc_buf[pos];
    pos += 1;

    auto remote = uc_buf[pos];
    pos += 1;

    auto ctx = trace_api::SpanContext(trace_id, span_id,
                                      trace_api::TraceFlags(flags), remote);

    return

        opentelemetry::nostd::shared_ptr<trace_api::Span>(
            std::make_shared<trace_api::DefaultSpan>(std::move(ctx)));
}

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
    template <typename... OtherArgs>
    traced_awaitable_frame(const uh::cluster::messenger_header& header,
                           OtherArgs&&...) noexcept
        : m_context(header.context) {}

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

    trace_span* span() noexcept { return m_span ? &*m_span : nullptr; }

private:
    std::optional<trace_span> m_span;
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
    template <typename... OtherArgs>
    traced_awaitable_frame(const uh::cluster::messenger_header& header,
                           OtherArgs&&...) noexcept
        : m_context(header.context) {}

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

    trace_span* span() noexcept { return m_span ? &*m_span : nullptr; }

private:
    std::optional<trace_span> m_span;
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
