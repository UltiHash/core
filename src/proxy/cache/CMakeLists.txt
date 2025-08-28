add_subdirectory(disk)

add_library(proxy_cache INTERFACE)
target_link_libraries(proxy_cache utils)

