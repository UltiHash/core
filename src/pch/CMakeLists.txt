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

add_library(uh_precompile_headers STATIC pch.cpp)

target_link_libraries(uh_precompile_headers
                      PUBLIC
                      Boost::log
                      Boost::url
                      Boost::filesystem
                      etcd-cpp-api
                      CLI11::CLI11
                      opentelemetry_exporter_otlp_grpc_log
                      opentelemetry_exporter_otlp_grpc
                      opentelemetry_exporter_otlp_grpc_metrics
                      opentelemetry_exporter_ostream_span
                      opentelemetry_exporter_ostream_metrics
                      zpp_bits
                      magic_enum
                      lmdbxx
                      reedsolomon
                      nlohmann_json)

target_precompile_headers(
    uh_precompile_headers
    PRIVATE
    <algorithm>
    <array>
    <atomic>
    <cctype>
    <charconv>
    <chrono>
    <concepts>
    <condition_variable>
    <csignal>
    <cstdint>
    <cstdlib>
    <cstring>
    <ctime>
    <deque>
    <exception>
    <filesystem>
    <format>
    <forward_list>
    <fstream>
    <functional>
    <iomanip>
    <iosfwd>
    <list>
    <map>
    <memory>
    <mutex>
    <optional>
    <ostream>
    <random>
    <ranges>
    <set>
    <shared_mutex>
    <source_location>
    <span>
    <sstream>
    <stdexcept>
    <string>
    <string_view>
    <thread>
    <tuple>
    <typeinfo>
    <unordered_map>
    <unordered_set>
    <utility>
    <variant>
    <vector>
    <cxxabi.h>
    <endian.h>
    <fcntl.h>
    <unistd.h>
    <CLI/CLI.hpp>
    <boost/algorithm/string.hpp>
    <boost/asio.hpp>
    <boost/beast.hpp>
    <boost/log/trivial.hpp>
    <boost/property_tree/ptree.hpp>
    <boost/url.hpp>
    <magic_enum/magic_enum.hpp>
    <openssl/bio.h>
    <openssl/core_names.h>
    <openssl/err.h>
    <openssl/evp.h>
    <openssl/kdf.h>
    <openssl/params.h>
    <openssl/pem.h>
    <nlohmann/json.hpp>
    <zpp_bits.h>)
