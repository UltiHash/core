# Copyright 2026 UltiHash Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

add_library(telemetry
            trace/trace.cpp
            log.cpp
            metrics.cpp
            otel_log_sink.cpp)

target_compile_options(telemetry PRIVATE -Wno-deprecated-builtins -Wno-deprecated-declarations)

target_link_libraries(telemetry
                      magic_enum
                      Boost::log
                      opentelemetry_exporter_otlp_grpc_log
                      opentelemetry_exporter_otlp_grpc
                      opentelemetry_exporter_otlp_grpc_metrics
                      opentelemetry_exporter_ostream_span
                      opentelemetry_exporter_ostream_metrics)
