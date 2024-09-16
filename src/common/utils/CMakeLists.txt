add_library(uh_utils xml_parser.cpp error.cpp strings.cpp temp_directory.cpp address_utils.cpp random.cpp host_utils.cpp common.cpp)
target_link_libraries(uh_utils uh_telemetry uh_service_interfaces uh_network uh_types)
