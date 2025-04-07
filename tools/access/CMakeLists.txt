project(uh-access)

add_executable(uh-access main.cpp)
target_link_libraries(uh-access db conf)
