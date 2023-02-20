cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)

project(uhClient VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 20)
message(STATUS "Using ${PROJECT_NAME}...")

# configure a header file to pass some of the CMake settings
# to the source code
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/project_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/project_config.h)

# external dependencies
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.80 COMPONENTS program_options unit_test_framework REQUIRED)
find_package(OpenSSL REQUIRED)

# putting internal project repository LIBUH_DEP in a variable
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
add_subdirectory(src/serialization)

# configuring library dependencies for cmake and linker
set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/src ${Boost_INCLUDE_DIRS} uhLibLogging uhLibOptions)
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} uhLibLogging uhLibProtocol uhLibOptions uhLibSerialization ${OPENSSL_LIBRARIES})

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/client_options/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.h)
# adding all the .h files of the current project
file(GLOB SOURCE_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/client_options/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.cpp)

# setting flags for optimizations
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

# creating a target executable
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