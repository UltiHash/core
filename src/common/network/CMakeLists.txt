add_library_bundle(network
    SOURCES messenger_core.cpp messenger.cpp tools.cpp
    PUBLIC zpp_bits telemetry cpr::cpr utils)

