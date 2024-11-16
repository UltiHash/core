add_library(uh_storage reference_counter.cpp data_store.cpp)
target_link_libraries(uh_storage uh_telemetry uh_registry uh_types uh_utils uh_network lmdbxx lmdb)
add_subdirectory(interfaces)
