add_subdirectory(prometheus-cpp)
add_subdirectory(pugixml)

add_library(zpp_bits INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/zpp_bits/zpp_bits.h)
target_include_directories(zpp_bits INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/zpp_bits)
