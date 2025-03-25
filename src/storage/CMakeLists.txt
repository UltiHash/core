add_library(storage
            handler.cpp
            interfaces/storage_group.cpp
            data_file.cpp
            default_data_store.cpp
            reference_counter.cpp)

target_link_libraries(storage utils types etcd lmdbxx ec) 
