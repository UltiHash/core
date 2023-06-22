cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)

project(uhLibLicensing)
message(STATUS "Using ${PROJECT_NAME}...")

set(LICENSE_SPRING_API_KEY "f43e779a-71bc-460f-af89-69a1c47cbe8b")
set(LICENSE_SPRING_SHARED_KEY "UtVbATx32BYf9QAQtLmcEJ4U5-58SezMIkeyb2Cy8l0")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/license_spring_credentials.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/license_spring_credentials.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# external dependencies
find_package(Boost 1.80 COMPONENTS system unit_test_framework json REQUIRED)
find_package(OpenSSL REQUIRED)

set(${PROJECT_NAME}_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} uhLibIo libLicenseSpring uhLibLogging)

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# adding all the .cpp files of the current project
file(GLOB SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${PROJECT_NAME} ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/testing/CMakeLists.txt)
