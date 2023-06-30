cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(uhLsUhv)
message(STATUS "Building ${PROJECT_NAME}...")

set(CMAKE_CXX_STANDARD 20)

# external dependencies
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.80 COMPONENTS regex date_time system filesystem unit_test_framework program_options REQUIRED)

# internal dependencies
set(${PROJECT_NAME}_INTERNAL_DEPS uhLibLogging uhLibUtil uhLibOptions uhLibIo uhLibChunking uhLibNet uhLibProtocol uhLibSerialization uhLibUhv)

# configuring library dependencies for cmake and linker
set(${PROJECT_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${Boost_INCLUDE_DIRS} ${${PROJECT_NAME}_INTERNAL_DEPS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${${PROJECT_NAME}_INTERNAL_DEPS})

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# adding all the .cpp files of the current project
file(GLOB SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(REMOVE_ITEM SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_LINK_LIBRARIES} )
