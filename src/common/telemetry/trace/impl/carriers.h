#pragma once

#include <concepts>
#include <opentelemetry/context/propagation/text_map_propagator.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <string>

template <typename T>
concept HttpRequestLike =
    requires(T message, const std::string& key, std::string_view value) {
        { message.set(key, value) };
        { message[key] } -> std::convertible_to<std::string_view>;
    };
template <HttpRequestLike Req>
class HttpRequestCarrier
    : public opentelemetry::context::propagation::TextMapCarrier {
public:
    HttpRequestCarrier(Req& headers)
        : m_headers(headers) {}
    HttpRequestCarrier() = default;

    virtual opentelemetry::nostd::string_view
    Get(opentelemetry::nostd::string_view key) const noexcept override {
        std::string key_to_compare = key.data();
        // Header's first letter seems to be  automatically capitaliazed by our
        // test http-server, so compare accordingly.
        if (key == opentelemetry::trace::propagation::kTraceParent) {
            key_to_compare = "traceparent";
        } else if (key == opentelemetry::trace::propagation::kTraceState) {
            key_to_compare = "tracestate";
        }
        auto value = m_headers[key_to_compare];
        return opentelemetry::nostd::string_view(value.data(), value.size());
    }

    virtual void
    Set(opentelemetry::nostd::string_view key,
        opentelemetry::nostd::string_view value) noexcept override {
        m_headers.set(std::string(key), std::string(value));
    }
    Req& m_headers;
};

template <typename T>
concept MapLike = requires(T message, typename T::key_type key,
                           typename T::mapped_type value) {
    typename T::key_type;
    typename T::mapped_type;

    {
        message.insert(
            std::pair<const typename T::key_type, typename T::mapped_type>(
                key, value))
    };
    { message.find(key) } -> std::same_as<decltype(message.end())>;
    { message.end() };
};

template <MapLike Map>
class HttpTextMapCarrier
    : public opentelemetry::context::propagation::TextMapCarrier {
public:
    HttpTextMapCarrier(Map& headers)
        : m_headers(headers) {}
    HttpTextMapCarrier() = default;
    virtual opentelemetry::nostd::string_view
    Get(opentelemetry::nostd::string_view key) const noexcept override {
        std::string key_to_compare = key.data();
        if (key == opentelemetry::trace::propagation::kTraceParent) {
            key_to_compare = "traceparent";
        } else if (key == opentelemetry::trace::propagation::kTraceState) {
            key_to_compare = "tracestate";
        }
        auto it = m_headers.find(key_to_compare);
        if (it != m_headers.end()) {
            return it->second;
        }
        return "";
    }

    virtual void
    Set(opentelemetry::nostd::string_view key,
        opentelemetry::nostd::string_view value) noexcept override {
        m_headers.insert(std::pair<std::string, std::string>(
            std::string(key), std::string(value)));
    }

    Map& m_headers;
};
