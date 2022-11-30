cmake_minimum_required(VERSION 3.24)
project(server-db VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

find_package(Boost COMPONENTS regex date_time system filesystem unit_test_framework program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

if(NOT EXISTS ${PROJECT_BINARY_DIR}/.gitignore)
    file(WRITE ${PROJECT_BINARY_DIR}/.gitignore "*")
endif()

add_subdirectory(lib)
add_subdirectory(src)
