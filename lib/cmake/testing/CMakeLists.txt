#Setup CMake to run tests
enable_testing()
include_directories (${Boost_INCLUDE_DIRS})

#Find all *.cpp files in the test folder
file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)
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
    add_executable(${PROJECT_NAME}_${testName} ${HEADER_FILE_LIST} ${testSrc})
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
add_executable (${PROJECT_NAME}_allTests ${HEADER_FILE_LIST} ${testSrcFiles} test/testRunner.cpp)
target_compile_definitions(${PROJECT_NAME}_allTests PUBLIC SINGLE_TEST_RUNNER=1)
target_include_directories(${PROJECT_NAME}_allTests PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries (${PROJECT_NAME}_allTests ${${PROJECT_NAME}_LINK_LIBRARIES} ${DEPENDENCIES})