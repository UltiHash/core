add_module(address_utils SOURCES address_utils.cpp PUBLIC_MODULE telemetry
           types)

add_module(common_utils SOURCES common.cpp)
add_module(error_utils SOURCES error.cpp)
add_module(host_utils SOURCES host_utils.cpp INTERFACE boost_headers)
add_module(io_utils SOURCES io.cpp)
add_module(misc_utils SOURCES misc.cpp INTERFACE project_includes)
add_module(
    random_utils
    SOURCES
    random.cpp
    SETTING
    boost_headers
    PUBLIC_MODULE
    types)
add_module(
    strings_utils
    SOURCES
    strings.cpp
    PRIVATE
    Boost::url)

# TODO: Move this to test/lib/util/
add_module(temp_directory_utils SOURCES temp_directory.cpp PRIVATE random_utils)

add_module(xml_parser_utils SOURCES xml_parser.cpp INTERFACE boost_headers)
