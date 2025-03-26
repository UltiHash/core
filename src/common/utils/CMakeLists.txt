add_library(utils
            common.cpp
            error.cpp
            host_utils.cpp
            io.cpp
            misc.cpp
            xml_parser.cpp
            random.cpp
            strings.cpp
            address_utils.cpp)

target_link_libraries(utils telemetry types Boost::url)
