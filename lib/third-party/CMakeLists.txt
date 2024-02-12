set(CMAKE_CXX_STANDARD 20)

option(BUILD_SHARED_LIBS "Build etcd-cpp-apiv3 shared libraries" OFF)
option(BUILD_ETCD_CORE_ONLY "Build etcd-cpp-apiv3 core library (the synchronous runtime) only" ON)
add_subdirectory(etcd-cpp-apiv3)

add_subdirectory(CLI11)

add_library(zpp_bits INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/zpp_bits/zpp_bits.h)
target_include_directories(zpp_bits INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/zpp_bits)

