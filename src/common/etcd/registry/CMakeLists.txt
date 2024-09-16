add_library(uh_registry service_registry.cpp service_id.cpp)
target_link_libraries(uh_registry uh_utils uh_network)
