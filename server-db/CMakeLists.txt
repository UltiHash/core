cmake_minimum_required(VERSION 3.22)
project(db-node VERSION 0.0.1)

add_subdirectory(lib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost 1.80 COMPONENTS regex date_time system filesystem unit_test_framework program_options REQUIRED)

set(${PROJECT_NAME}_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
set(${PROJECT_NAME}_LINK_LIBRARIES uhLibOptions uhLibLogging uhLibNetwork uhprotocol ${Boost_LIBRARIES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib) # How to add this to ${PROJECT_NAME}_INCLUDE_DIRS?
include_directories(${PROJECT_NAME}_INCLUDE_DIRS)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

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

    # --- auto-ignore build directory
    if(NOT EXISTS ${PROJECT_BINARY_DIR}/.gitignore)
        file(WRITE ${PROJECT_BINARY_DIR}/.gitignore "*")
    endif()
endif()


###

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/src/config.hpp)

# adding all the .cpp to the sources and .h to the headers of the current project
file(GLOB SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)


add_executable(${PROJECT_NAME}  ${SOURCE_FILE_LIST})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

#We can refer to uhLibOptions, etc... because they are defined as PUBLIC in their respective Cmake projects.
target_link_libraries(${PROJECT_NAME} ${${PROJECT_NAME}_LINK_LIBRARIES})
###



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
    #link to Boost libraries AND your targets and dependencies
    target_link_libraries(${PROJECT_NAME}_${testName} ${${PROJECT_NAME}_LINK_LIBRARIES} ${DEPENDENCIES})
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
target_link_libraries (${PROJECT_NAME}_allTests ${${PROJECT_NAME}_LINK_LIBRARIES} ${DEPENDENCIES})
