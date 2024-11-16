add_library(uh_utils STATIC address_utils.cpp strings.cpp xml_parser.cpp error.cpp misc.cpp temp_directory.cpp host_utils.cpp common.cpp random.cpp)
target_link_libraries(uh_utils PRIVATE uh_telemetry uh_service_interfaces uh_types uh_network PUBLIC Boost::url uh_types uh_etcd)
