add_library(uh_storage reference_counter.cpp data_store.cpp)
target_link_libraries(uh_storage
PRIVATE
uh_telemetry
PUBLIC
uh_coroutines
lmdbxx
lmdb
uh_storage_interfaces
uh_registry
uh_debug
uh_utils
uh_types
uh_network
)
add_subdirectory(interfaces)
