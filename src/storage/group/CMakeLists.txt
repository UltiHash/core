add_library(storage_group config.cpp state_context.cpp)
target_link_libraries(storage_group nlohmann_json etcd utils)
