find_package(Python3 COMPONENTS Interpreter)

set(enable_e2e_test ${ENABLE_E2E_TESTS})

if (NOT Python3_Interpreter_FOUND)

    message(WARNING "Python3 could not be found. Extended testing via PyTest is disabled.")
    return()

endif ()

find_program(Docker_EXECUTABLE docker)
if(NOT Docker_EXECUTABLE)

    message(WARNING "Cannot find the docker executable. E2E tests will not run during `make test`.")
    set(enable_e2e_test OFF)

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
        INPUT ${CMAKE_CURRENT_SOURCE_DIR}/docker-compose.yml.in)

endif()

set(TESTING_VENV_DIR "${CMAKE_CURRENT_BINARY_DIR}/venv")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/start-e2e.sh.in
            ${CMAKE_CURRENT_BINARY_DIR}/start-e2e.sh
            FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pytest.ini.in
        ${CMAKE_CURRENT_BINARY_DIR}/pytest.ini)

file(CREATE_LINK
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_BINARY_DIR}/tests
        SYMBOLIC)

add_test(NAME pytest_e2e
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/start-e2e.sh)

set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES "${TESTING_VENV_DIR}")

if (enable_e2e_test)
    add_test(NAME pytest_e2e
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/start-e2e.sh)
endif()
