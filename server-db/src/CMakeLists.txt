configure_file(config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)

# adding all the .cpp to the sources and .h to the headers of the current project
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${PROJECT_NAME} uhLibOptions uhLibLogging uhLibNetwork uhprotocol ${Boost_LIBRARIES})
