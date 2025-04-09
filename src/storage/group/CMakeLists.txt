add_library(storage_group config.cpp state.cpp)
target_link_libraries(storage_group nlohmann_json etcd utils)
