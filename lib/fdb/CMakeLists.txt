cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)

project(uhLibFdb)
message(STATUS "Using ${PROJECT_NAME}...")

# external dependencies
find_package(Boost 1.80 COMPONENTS unit_test_framework REQUIRED)
if(APPLE)
    find_library(FDB_C_LIB NAMES fdb_c PATHS /usr/local/lib)
    add_library(fdb_c UNKNOWN IMPORTED)
    set_property(TARGET fdb_c PROPERTY IMPORTED_LOCATION ${FDB_C_LIB})
else()
    find_package(FoundationDB-Client REQUIRED)
endif()

set(${PROJECT_NAME}_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} uhLibUtil fdb_c)

# adding all the .h files of the current project
set(HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/fdb.h)

# adding all the .cpp files of the current project
set(SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/fdb.cpp)

add_library(${PROJECT_NAME} OBJECT ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/testing/CMakeLists.txt)
