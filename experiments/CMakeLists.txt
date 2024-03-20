cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_executable(${PROJECT_NAME}-dd-prefix-expr dedupe_prefix_scale.cpp)
add_executable(${PROJECT_NAME}-dd-dumb-expr dedupe_dumb_scale.cpp)
add_executable(${PROJECT_NAME}-dd-prefix-isolate-expr dedupe_prefix_isolate_scale.cpp)

add_executable(${PROJECT_NAME}-basic-client client/basic/main.cpp)
target_link_libraries(${PROJECT_NAME}-basic-client ${PROJECT_NAME}_OBJECTS ${${PROJECT_NAME}_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME}-basic-client PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})

add_executable(${PROJECT_NAME}-storage-stress-test client/storage_stress_test/main.cpp)
target_link_libraries(${PROJECT_NAME}-storage-stress-test ${PROJECT_NAME}_OBJECTS ${${PROJECT_NAME}_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME}-storage-stress-test PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})