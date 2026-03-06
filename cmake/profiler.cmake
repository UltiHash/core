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

include(CPM)

CPMAddPackage(
    NAME
    benchmark
    GITHUB_REPOSITORY
    google/benchmark
    VERSION
    1.9.4
    OPTIONS
    "BENCHMARK_ENABLE_TESTING Off"
    CPM_ARGS
    "TIMEOUT 300")

function(uh_add_profiler name)
    # Parse Arguments
    set(options "")
    set(multi_value_args PRIVATE PUBLIC)
    cmake_parse_arguments(ARGS "${options}" "${one_value_args}"
                          "${multi_value_args}" ${ARGN})

    set(target_name "${name}")

    add_executable(${target_name} ${name}.cpp)

    target_link_libraries(
        ${target_name}
        PRIVATE benchmark::benchmark_main ${ARGS_PRIVATE}
        PUBLIC ${ARGS_PUBLIC})

    target_precompile_headers(${target_name} REUSE_FROM uh_precompile_headers)
endfunction()


