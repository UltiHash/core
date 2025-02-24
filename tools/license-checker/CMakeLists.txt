add_executable(license-checker main.cpp)
target_link_libraries(license-checker ${PROJECT_NAME}_shared
                      ${${PROJECT_NAME}_LINK_LIBRARIES})
target_include_directories(license-checker
                           PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})
