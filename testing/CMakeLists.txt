find_package(Python3 COMPONENTS Interpreter)

if (NOT Python3_Interpreter_FOUND)

    if (${ENABLE_INTEGRATION_TESTS})
        message(FATAL_ERROR "Python3 could not be found. Extended testing via PyTest is disabled.")
    else()
        message(WARNING "Python3 could not be found. Extended testing via PyTest is disabled.")
    endif()

    return()

endif ()

find_program(Docker_EXECUTABLE docker)
if(NOT Docker_EXECUTABLE)

    message(WARNING "Cannot find the docker executable. E2E tests will not run during `make test`.")

else()

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/docker-image
        COMMAND ${Docker_EXECUTABLE} build --tag uh/test ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/docker-image
        COMMENT "Create a file tracking the last time the docker image was updated"
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile)

    add_custom_target(TestDockerContainer ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/docker-image)

    file(GENERATE
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/docker-compose.yml
        INPUT ${CMAKE_CURRENT_SOURCE_DIR}/docker-compose.yml)

endif()

set(TESTING_VENV_DIR "${CMAKE_CURRENT_BINARY_DIR}/venv")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/start-e2e.sh
            ${CMAKE_CURRENT_BINARY_DIR}/start-e2e.sh
            FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pytest.ini
        ${CMAKE_CURRENT_BINARY_DIR}/pytest.ini)

file(CREATE_LINK
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_BINARY_DIR}/tests
        SYMBOLIC)

file(CREATE_LINK
        ${CMAKE_CURRENT_SOURCE_DIR}/s3-tests
        ${CMAKE_CURRENT_BINARY_DIR}/s3-tests
        SYMBOLIC)

set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES "${TESTING_VENV_DIR}")

add_executable(${PROJECT_NAME}-client ${CMAKE_CURRENT_SOURCE_DIR}/client/main.cpp)
target_link_libraries(${PROJECT_NAME}-client ${PROJECT_NAME}_OBJECTS ${${PROJECT_NAME}_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME}-client PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})


add_test(NAME pytest_e2e
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/start-e2e.sh)
