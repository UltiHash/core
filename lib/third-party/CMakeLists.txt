
option(BUILD_SHARED_LIBS "Build etcd-cpp-apiv3 shared libraries" OFF)
add_subdirectory(etcd-cpp-apiv3)

add_library(zpp_bits INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/zpp_bits/zpp_bits.h)
target_include_directories(zpp_bits INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/zpp_bits)

