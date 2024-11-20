add_library(
    uh_utils STATIC
    address_utils.cpp
    strings.cpp
    xml_parser.cpp
    error.cpp
    misc.cpp
    temp_directory.cpp
    host_utils.cpp
    common.cpp
    random.cpp)
target_link_libraries(
    uh_utils
    # TODO: Check if these headers are private or public
    uh_telemetry
    uh_service_interfaces
    uh_types
    uh_network
    Boost::url
    uh_types
    uh_etcd)
target_precompile_headers(uh_utils REUSE_FROM uh_precompile_headers)
