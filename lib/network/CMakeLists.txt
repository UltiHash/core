cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 23)

project(uhLibNetwork)
message(STATUS "Using ${PROJECT_NAME}...")

# external dependencies
find_package(Boost 1.80 COMPONENTS thread system unit_test_framework REQUIRED)
find_package(OpenSSL REQUIRED)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include ${Boost_INCLUDE_DIRS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})

# adding all the .h files of the current projectsimple_server.cpp
file(GLOB HEADER_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
        )

add_library(${PROJECT_NAME} INTERFACE ${HEADER_FILE_LIST})
target_include_directories(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_LINK_LIBRARIES})

# special case for building executables for test server / client executables
add_executable(simple_server ${HEADER_FILE_LIST} src/simple_server.cpp)
target_include_directories(simple_server PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(simple_server PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES} ${DEPENDENCIES})

add_executable(simple_client ${HEADER_FILE_LIST} src/simple_client.cpp)
target_include_directories(simple_client PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(simple_client PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES} ${DEPENDENCIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/testing/CMakeLists.txt)