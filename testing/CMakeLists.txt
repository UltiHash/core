find_package(Python3 COMPONENTS Interpreter)

if (Python3_Interpreter_FOUND)

    set(TESTING_VENV_DIR "${CMAKE_CURRENT_BINARY_DIR}/venv")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/start-e2e.sh.in
                ${CMAKE_CURRENT_BINARY_DIR}/start-e2e.sh
                FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE)

    add_test(NAME pytest_e2e
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/start-e2e.sh)

    set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES "${TESTING_VENV_DIR}")

else ()

    message(WARNING "Python3 could not be found. Extended testing via PyTest is disabled")

endif ()
