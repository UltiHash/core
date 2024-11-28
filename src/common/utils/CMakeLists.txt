add_static_and_shared(
    uh_utils
    SOURCES
    strings.cpp
    xml_parser.cpp
    error.cpp
    misc.cpp
    temp_directory.cpp
    random.cpp
    # TODO: Check if these headers are private or public
    PRIVATE
    Boost::url
    Boost::log
    PUBLIC
    # uh_etcd
    # uh_service_interfaces
    # uh_types
    # uh_telemetry
    # uh_network
)
