add_library(group_data config.cpp)
target_link_libraries(group_data nlohmann_json etcd utils)