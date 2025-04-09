add_subdirectory(group)

add_library(storage
            address_utils.cpp
            handler.cpp
            interfaces/remote_storage.cpp
            global_data/default_global_data_view.cpp
            data_file.cpp
            default_data_store.cpp
            reference_counter.cpp)

target_link_libraries(storage storage_group utils types etcd lmdbxx license)
