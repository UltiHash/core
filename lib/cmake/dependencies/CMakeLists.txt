#find_package(Git REQUIRED)
#if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
#    # Update submodules as needed
#    option(GIT_SUBMODULE "Check submodules during build" ON)
#    if(GIT_SUBMODULE)
#        message(STATUS "Submodule update")
#        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --remote
#                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#                RESULT_VARIABLE GIT_SUBMOD_RESULT)
#        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
#            message(FATAL_ERROR "git submodule update --init --recursive --remote failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
#        endif()
#    endif()
#endif()


foreach(DEPENDENCY ${DEPENDENCIES})
    if (NOT TARGET ${DEPENDENCY})
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../${DEPENDENCY} ${CMAKE_CURRENT_BINARY_DIR}/${DEPENDENCY})
    endif ()

    get_target_property(TARGET_INCLUDES ${DEPENDENCY} INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT TARGET_INCLUDES)
        message(WARNING "The include paths of the submodule ${DEPENDENCY} could not be resolved.")
    else()
        list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS ${TARGET_INCLUDES})
    endif()

    get_target_property(TARGET_LINK_LIBRARIES ${DEPENDENCY} INTERFACE_LINK_LIBRARIES)
    if(NOT TARGET_LINK_LIBRARIES)
        message(WARNING "The link libraries of the submodule ${DEPENDENCY} could not be resolved.")
    else()
        list(APPEND ${PROJECT_NAME}_LINK_LIBRARIES ${TARGET_LINK_LIBRARIES})
    endif()
endforeach(DEPENDENCY)