add_library_bundle(utils
    SOURCES common.cpp error.cpp host_utils.cpp io.cpp misc.cpp xml_parser.cpp
            random.cpp
    PUBLIC boost_headers)

add_library_bundle(
    strings_utils
    SOURCES
    strings.cpp
    PRIVATE
    Boost::url)

