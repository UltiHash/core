
find_library(READLINE_LIBRARY readline)

if (READLINE_LIBRARY)
    add_executable(dd-shell main.cpp)
    target_link_libraries(dd-shell ${PROJECT_NAME}_shared ${${PROJECT_NAME}_LINK_LIBRARIES} readline)
    target_include_directories(dd-shell PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})
endif()
