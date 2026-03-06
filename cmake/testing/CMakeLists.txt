# Copyright 2026 UltiHash Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

######### TESTING #######
#Setup CMake to run tests
enable_testing()

#Find all *.cpp files in the test folder
file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)

#Create a stand-alone executable for each one of them,
#as having multiple test binaries to be evaluated using ctest upon invoking "make test"
#enables more fine grained tests in a CI pipeline.
foreach(testSrc ${TEST_SRCS})
    #Extract the filename without an extension (NAME_WE)
    get_filename_component(testName ${testSrc} NAME_WE)
    #Add compile target
    add_executable(${PROJECT_NAME}_${testName} ${testSrc})
    #set include directory of this project
    target_include_directories(${PROJECT_NAME}_${testName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/)
    #link to external dependencies and targets from lib-common
    get_target_property(target_type ${PROJECT_NAME} TYPE)
    if (target_type STREQUAL "EXECUTABLE")
        # Use object library target instead
        target_link_libraries(${PROJECT_NAME}_${testName} PRIVATE ${PROJECT_NAME}_OBJECTS ${${PROJECT_NAME}_LINK_LIBRARIES})
    else()
        target_link_libraries(${PROJECT_NAME}_${testName} PRIVATE ${PROJECT_NAME} ${${PROJECT_NAME}_LINK_LIBRARIES})
    endif ()

    #make sure to register the test binary for the "make test" target
    add_test(NAME ${PROJECT_NAME}_${testName}
            COMMAND ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}_${testName} )
    set_tests_properties(${PROJECT_NAME}_${testName} PROPERTIES TIMEOUT 600)
endforeach(testSrc)