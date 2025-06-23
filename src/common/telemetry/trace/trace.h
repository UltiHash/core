#pragma once

#include <opentelemetry/context/propagation/global_propagator.h>

#include "impl/carriers.h"

namespace uh::cluster {

inline constexpr std::string_view TRACE_STDOUT_ENDPOINT = "stdout";

void initialize_trace(const std::string& tracer_name,
                      const std::string& tracer_version,
                      const std::string& endpoint = "localhost:4317");

template <HttpRequestLike Req>
void encode_context(Req& req, const opentelemetry::context::Context& context) {
    HttpRequestCarrier carrier(req);

    auto propagator = opentelemetry::context::propagation::
        GlobalTextMapPropagator::GetGlobalPropagator();

    propagator->Inject(carrier, context);
}

template <HttpRequestLike Req>
opentelemetry::context::Context decode_context(Req& req) {

    HttpRequestCarrier carrier(req);

    auto propagator = opentelemetry::context::propagation::
        GlobalTextMapPropagator::GetGlobalPropagator();

    opentelemetry::context::Context empty_context;
    return propagator->Extract(carrier, empty_context);
}

template <MapLike Map>
void encode_context(Map& map, const opentelemetry::context::Context& context) {
    HttpTextMapCarrier carrier(map);

    auto propagator = opentelemetry::context::propagation::
        GlobalTextMapPropagator::GetGlobalPropagator();

    propagator->Inject(carrier, context);
}

template <MapLike Map>
opentelemetry::context::Context decode_context(Map& map) {

    HttpTextMapCarrier carrier(map);

    auto propagator = opentelemetry::context::propagation::
        GlobalTextMapPropagator::GetGlobalPropagator();

    opentelemetry::context::Context empty_context;
    return propagator->Extract(carrier, empty_context);
}

template <std::size_t N>
constexpr std::size_t constexpr_strlen(const char (&str)[N]) {
    return N - 1;
}

constexpr auto get_encoded_context_len() {
    constexpr auto length = constexpr_strlen(
        "00-996c6ce7ece3dcf1d2acfb7b89421fd6-28a16557cb531132-01");
    return length;
}

std::string encode_context(const opentelemetry::context::Context& context);
opentelemetry::context::Context decode_context(std::string traceparent);

} // namespace uh::cluster
