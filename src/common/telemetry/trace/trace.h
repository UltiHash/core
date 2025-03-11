#pragma once

#include <opentelemetry/context/propagation/global_propagator.h>

#include "impl/carriers.h"

void initialize_trace(const std::string& endpoint = "localhost:4317");

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

inline std::string
encode_context(const opentelemetry::context::Context& context) {
    std::map<std::string, std::string> map;
    encode_context(map, context);
    return map["traceparent"];
}
inline opentelemetry::context::Context decode_context(std::string traceparent) {
    std::map<std::string, std::string> map;
    map["traceparent"] = traceparent;
    return decode_context(map);
}
