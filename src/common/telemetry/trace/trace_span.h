#pragma once

#include <chrono>
#include <opentelemetry/context/context.h>
#include <opentelemetry/trace/context.h>
#include <opentelemetry/trace/provider.h>
#include <source_location>

namespace boost {
namespace asio {
template <typename, typename> class traced_awaitable;
namespace this_coro {
struct span_t;
}
namespace detail {
template <typename, typename> class traced_awaitable_frame;
}
} // namespace asio
} // namespace boost

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

    trace_span(const std::source_location& location) noexcept
        : m_location{location},
          m_start_system_clock{std::chrono::system_clock::now()},
          m_start_steady_clock{std::chrono::steady_clock::now()} {
        set_name(coroutine_name());
    }

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
            m_data->SetAttribute(m_span_name, value);
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

    std::string m_span_name;
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
                tracer_name);
            m_data = tracer->StartSpan(m_span_name, options);
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
