cmake_minimum_required(VERSION 3.22.0)
project(lib-options VERSION 0.0.1)

find_package(Boost 1.80 COMPONENTS program_options REQUIRED)

add_library(uhoptions basic_options.cpp options.cpp)
target_include_directories(uhoptions PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(uhoptions ${Boost_LIBRARIES})
