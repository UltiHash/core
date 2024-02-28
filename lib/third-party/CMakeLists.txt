set(CMAKE_CXX_STANDARD 20)

option(BUILD_SHARED_LIBS "Build etcd-cpp-apiv3 shared libraries" OFF)
option(BUILD_ETCD_CORE_ONLY "Build etcd-cpp-apiv3 core library (the synchronous runtime) only" ON)
add_subdirectory(etcd-cpp-apiv3)

add_subdirectory(CLI11)

option(WITH_OTLP_GRPC "Whether to include the OTLP gRPC exporter in the SDK" ON)
option(BUILD_TESTING "Whether to enable tests" OFF)
option(BUILD_SHARED_LIBS "Whether to build as shared library" OFF)
option(WITH_EXAMPLES "Whether to include examples in build" OFF)

add_subdirectory(opentelemetry-cpp)

add_library(magic_enum INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/magic_enum/include/magic_enum/magic_enum_all.hpp)
target_include_directories(magic_enum INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/magic_enum/include/)

add_library(zpp_bits INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/zpp_bits/zpp_bits.h)
target_include_directories(zpp_bits INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/zpp_bits)

