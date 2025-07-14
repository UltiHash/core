add_subdirectory(interfaces)
add_subdirectory(group)
add_subdirectory(global)

add_library(storage
            handler.cpp
            interfaces/remote_storage.cpp
            data_file.cpp
            default_data_store.cpp
            reference_counter.cpp)

target_link_libraries(storage group_data global_data utils types etcd lmdbxx license storage_interfaces)
