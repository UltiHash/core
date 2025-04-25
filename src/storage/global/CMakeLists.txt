add_library(global_data address_utils.cpp cache.cpp data_view.cpp)
target_link_libraries(global_data nlohmann_json etcd utils)
