cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)

add_executable(${PROJECT_NAME}-dd-prefix-expr dedupe_prefix_scale.cpp)
add_executable(${PROJECT_NAME}-dd-dumb-expr dedupe_dumb_scale.cpp)
add_executable(${PROJECT_NAME}-dd-prefix-isolate-expr dedupe_prefix_isolate_scale.cpp)
