cmake_minimum_required(VERSION 3.1)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_subdirectory(lib)
add_subdirectory(server-agency)
add_subdirectory(server-db)
add_subdirectory(client-shell)

enable_testing()
