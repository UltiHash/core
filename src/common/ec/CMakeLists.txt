
add_library(ec INTERFACE)
target_include_directories(ec INTERFACE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(ec INTERFACE reedsolomon)
