add_library(uh_storage data_store.cpp reference_counter.cpp)
target_link_libraries(uh_storage uh_telemetry uh_utils uh_registry uh_network uh_types lmdbxx)
add_subdirectory(interfaces)
