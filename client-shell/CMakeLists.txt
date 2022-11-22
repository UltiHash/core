cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)

project(CLIENT_SHELL VERSION 1.0)
message(STATUS "Using ${PROJECT_NAME}...")

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
        "client_shell_ver.h.in"
        "${PROJECT_BINARY_DIR}/client_shell_ver.h"  )

# putting internal project repository dependency in a variable
set(DEPENDENCY "lib-common")

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

# external dependencies
find_package(Boost 1.80 COMPONENTS thread system unit_test_framework REQUIRED)
find_package(OpenSSL REQUIRED)

# creating a target executable
add_executable(${PROJECT_NAME} src/main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_BINARY_DIR} ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})