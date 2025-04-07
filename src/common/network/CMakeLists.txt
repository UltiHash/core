add_library(network messenger_core.cpp messenger.cpp tools.cpp)
target_link_libraries(network zpp_bits telemetry cpr::cpr utils)
