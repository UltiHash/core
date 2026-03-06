// Copyright 2026 UltiHash Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define BOOST_TEST_MODULE "opentelemetry tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/telemetry/trace/trace.h>
#include <common/telemetry/trace/trace_asio.h>

namespace uh::cluster::storage {

BOOST_AUTO_TEST_SUITE(a_context)

BOOST_AUTO_TEST_CASE(stores_and_retrieves_custom_value) {

    auto context = opentelemetry::context::Context();
    context = context.SetValue("peer_port", static_cast<uint64_t>(11));

    auto peer_port = context.GetValue("peer_port");
    BOOST_TEST(true == std::holds_alternative<uint64_t>(peer_port));
    BOOST_TEST(11 == std::get<uint64_t>(peer_port));
}

BOOST_AUTO_TEST_CASE(is_accessible_after_creating_subspan) {

    auto context = opentelemetry::context::Context();
    context = context.SetValue("peer_port", static_cast<uint64_t>(11));

    auto parent_span = opentelemetry::trace::Provider::GetTracerProvider()
                           ->GetTracer("test_tracer")
                           ->StartSpan("parent_span", {.parent = context});

    context = opentelemetry::trace::SetSpan(context, parent_span);

    auto sub_span = opentelemetry::trace::Provider::GetTracerProvider()
                        ->GetTracer("test_tracer")
                        ->StartSpan("sub_span", {.parent = context});

    context = opentelemetry::trace::SetSpan(context, sub_span);

    auto peer_port = context.GetValue("peer_port");
    BOOST_TEST(true == std::holds_alternative<uint64_t>(peer_port));
    BOOST_TEST(11 == std::get<uint64_t>(peer_port));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
