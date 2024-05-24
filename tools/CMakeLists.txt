cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_executable(${PROJECT_NAME}-basic-client basic/main.cpp)
target_link_libraries(${PROJECT_NAME}-basic-client ${PROJECT_NAME}_OBJECTS ${${PROJECT_NAME}_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME}-basic-client PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})

add_executable(${PROJECT_NAME}-storage-stress-test storage_stress_test/main.cpp)
target_link_libraries(${PROJECT_NAME}-storage-stress-test ${PROJECT_NAME}_OBJECTS ${${PROJECT_NAME}_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME}-storage-stress-test PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})

add_executable(${PROJECT_NAME}-dd-stress-test dd_stress_test/main.cpp)
target_link_libraries(${PROJECT_NAME}-dd-stress-test ${PROJECT_NAME}_OBJECTS ${${PROJECT_NAME}_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME}-dd-stress-test PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})

add_executable(${PROJECT_NAME}-file-upload file_upload/main.cpp)
target_link_libraries(${PROJECT_NAME}-file-upload ${PROJECT_NAME}_OBJECTS ${${PROJECT_NAME}_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME}-file-upload PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})

add_executable(${PROJECT_NAME}-uh-dir uh-dir/main.cpp)
target_link_libraries(${PROJECT_NAME}-uh-dir ${PROJECT_NAME}_OBJECTS ${${PROJECT_NAME}_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME}-uh-dir PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})
