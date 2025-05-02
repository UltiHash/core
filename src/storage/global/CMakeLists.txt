add_library(global_data cache.cpp data_view.cpp)
target_link_libraries(global_data nlohmann_json etcd utils group_data)
