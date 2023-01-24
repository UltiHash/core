cmake_minimum_required(VERSION 3.22.0)
project(uhServerDB VERSION 0.2.0)
message(STATUS "Building ${PROJECT_NAME}...")

set(CMAKE_CXX_STANDARD 20)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.80 COMPONENTS unit_test_framework REQUIRED)

find_package(Git REQUIRED)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    message(STATUS "Reading git revision")

    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_REVISION)
    string(STRIP "${GIT_REVISION}" PROJECT_VCSID)

    execute_process(COMMAND ${GIT_EXECUTABLE} remote get-url origin
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_REPOSITORY)
    string(STRIP "${GIT_REPOSITORY}" PROJECT_REPOSITORY)

    # auto-ignore build directory
    if(NOT EXISTS ${PROJECT_BINARY_DIR}/.gitignore)
        file(WRITE ${PROJECT_BINARY_DIR}/.gitignore "*")
    endif()

    # Update submodules as needed
    option(UPDATE_GIT_SUBMODULE "Update submodules during build" OFF)
    if(UPDATE_GIT_SUBMODULE)
        message(STATUS "Updating submodules...")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --remote
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive --remote failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(lib)

set(${PROJECT_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/lib ${CMAKE_CURRENT_SOURCE_DIR}/src ${Boost_INCLUDE_DIRS} uhLibOptions uhLibLogging uhLibProtocol uhLibNet)
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} uhLibOptions uhLibLogging uhLibProtocol uhLibNet uhLibMetrics)

# adding all the .h files of the current project
file(GLOB_RECURSE HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

# adding all the .h files of the current project
file(GLOB_RECURSE SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(${PROJECT_NAME} ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_LINK_LIBRARIES})


######### TESTING #######
#Setup CMake to run tests
enable_testing()

list(REMOVE_ITEM SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

#Find all *.cpp files in the test folder
file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)
#message( WARNING "Current src dir : ${CMAKE_CURRENT_SOURCE_DIR}")
#Initialize list of all test source files
set(testSrcFiles "")

#Create a stand-alone executable for each one of them,
#as having multiple test binaries to be evaluated using ctest upon invoking "make test"
#enables more fine grained tests in a CI pipeline.
foreach(testSrc ${TEST_SRCS})
    #skip over the testRunner.cpp, as this is the driver for accommodating
    #all test cases in a single binary (see further below)
    if(${testSrc} STREQUAL "test/testRunner.cpp")
        continue()
    endif()
    #Extract the filename without an extension (NAME_WE)
    get_filename_component(testName ${testSrc} NAME_WE)
    #Add compile target
    add_executable(${PROJECT_NAME}_${testName} ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST} ${testSrc})
    #set include directory of this project
    target_include_directories(${PROJECT_NAME}_${testName} PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
    #link to external dependencies and targets from lib-common
    target_link_libraries(${PROJECT_NAME}_${testName} ${${PROJECT_NAME}_LINK_LIBRARIES})
    #make sure to register the test binary for the "make test" target
    add_test(NAME ${PROJECT_NAME}_${testName}
            COMMAND ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}_${testName} )
    #store source file name for single test executable scenario (see below)
    list(APPEND testSrcFiles ${testSrc})
endforeach(testSrc)

# creating a single test executable facilitates a better integration between boost.test and CLion
add_executable (${PROJECT_NAME}_allTests ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST} ${testSrcFiles} test/testRunner.cpp)
target_compile_definitions(${PROJECT_NAME}_allTests PUBLIC SINGLE_TEST_RUNNER=1)
target_include_directories(${PROJECT_NAME}_allTests PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries (${PROJECT_NAME}_allTests ${${PROJECT_NAME}_LINK_LIBRARIES})

