project(storage)

add_executable(uh-storage main.cpp)
target_link_libraries(uh-storage uh_entrypoint uh_storage)
