add_library_bundle(utils
    SOURCES common.cpp error.cpp host_utils.cpp io.cpp misc.cpp xml_parser.cpp
            random.cpp strings.cpp address_utils.cpp
    PUBLIC telemetry types Boost::url)
