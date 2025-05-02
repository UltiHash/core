add_library(storage_interfaces 
    remote_storage.cpp
)
target_link_libraries(storage_interfaces network service_interfaces)

