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

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${UH_BINARY_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${UH_BINARY_OUTPUT_DIR})

add_subdirectory(pch)
add_subdirectory(common)
add_subdirectory(config)
add_subdirectory(storage)
add_subdirectory(deduplicator)
add_subdirectory(entrypoint)
add_subdirectory(coordinator)
add_subdirectory(proxy)

add_executable(uh-cluster main.cpp)
target_link_libraries(uh-cluster Threads::Threads types telemetry conf entrypoint deduplicator storage coordinator proxy)
target_precompile_headers(uh-cluster REUSE_FROM uh_precompile_headers)
