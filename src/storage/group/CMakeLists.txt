add_library(group_data
    impl/address_utils.cpp
    config.cpp
    rr_data_view.cpp
    ec_data_view.cpp
)
target_link_libraries(group_data nlohmann_json etcd utils storage_interfaces)
