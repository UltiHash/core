cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)

project(CLIENT_SHELL VERSION 1.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "Using ${PROJECT_NAME}...")

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
        "client_shell_ver.h.in"
        "${PROJECT_BINARY_DIR}/client_shell_ver.h"  )

# downloading the submodule
find_package(Git REQUIRED)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src
                RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive --remote failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

# putting internal project repository LIBUH_DEP in a variable
set(LIBUH_DEP "lib-common")

# external dependencies
find_package(Boost 1.80 COMPONENTS thread system log unit_test_framework REQUIRED)
find_package(OpenSSL REQUIRED)
find_library(PQXX_LIB pqxx REQUIRED)
find_library(PQ_LIB pq REQUIRED)

# adding all the .h files of the current project
file(GLOB HEADERSRC_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/${LIBUH_DEP}/dbSupport/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/${LIBUH_DEP}/dbSupport/*.cpp
        )


# creating a target executable
add_executable(${PROJECT_NAME} ${HEADERSRC_FILE_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_BINARY_DIR} ${Boost_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/${LIBUH_DEP} ${POSTGRE_ROOT})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${PQXX_LIB} ${PQ_LIB})