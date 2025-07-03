add_library(coordinator INTERFACE)
target_include_directories(coordinator INTERFACE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(coordinator INTERFACE license)
