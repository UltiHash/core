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

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(CLI11)

option(WITH_OTLP_GRPC "Whether to include the OTLP gRPC exporter in the SDK" ON)
option(BUILD_TESTING "Whether to enable tests" OFF)
option(BUILD_SHARED_LIBS "Whether to build as shared library" OFF)
option(WITH_EXAMPLES "Whether to include examples in build" OFF)
option(WITH_STL "Which version of the Standard Library for C++ to use" ON)

add_subdirectory(opentelemetry-cpp)
add_subdirectory(nlohmann_json)

add_library(magic_enum INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/magic_enum/include/magic_enum/magic_enum_all.hpp)
target_include_directories(magic_enum INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/magic_enum/include/)

add_library(zpp_bits INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/zpp_bits/zpp_bits.h)
target_include_directories(zpp_bits INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/zpp_bits)

find_library(LMDB_LIBRARY NAMES lmdb REQUIRED)
add_library(lmdbxx INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lmdbxx/include/lmdbxx/lmdb++.h)
target_include_directories(lmdbxx INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lmdbxx/include)
target_link_libraries(lmdbxx INTERFACE lmdb)

enable_language(C)
set(ReedSolomonC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/reedsolomon-c)
add_library(reedsolomon STATIC ${ReedSolomonC_DIR}/rs.c)
target_include_directories(reedsolomon PUBLIC ${ReedSolomonC_DIR})
