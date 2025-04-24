add_library(group_data config.cpp state_context.cpp)
target_link_libraries(group_data nlohmann_json etcd utils)
