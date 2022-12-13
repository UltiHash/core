cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)

project(uhLibNet)
message(STATUS "Using ${PROJECT_NAME}...")

# external dependencies
find_package(Boost 1.80 COMPONENTS thread system unit_test_framework REQUIRED)
find_package(OpenSSL REQUIRED)

set(${PROJECT_NAME}_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} uhLibLogging uhLibUtil)

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(${PROJECT_NAME} ${HEADER_FILE_LIST} server.cpp scheduler.cpp socket.cpp plain_socket.cpp tls_socket.cpp tls_server.cpp)
target_include_directories(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${${PROJECT_NAME}_LINK_LIBRARIES})

# special case for building executables for test server / client executables
add_executable(${PROJECT_NAME}_simple_server ${HEADER_FILE_LIST} simple_server.cpp)
target_include_directories(${PROJECT_NAME}_simple_server PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}_simple_server PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES} ${DEPENDENCIES})

add_executable(${PROJECT_NAME}_simple_client ${HEADER_FILE_LIST} simple_client.cpp)
target_include_directories(${PROJECT_NAME}_simple_client PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}_simple_client PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES} ${DEPENDENCIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/testing/CMakeLists.txt)
