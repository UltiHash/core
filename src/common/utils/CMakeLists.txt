# TODO: Now address_utils.cpp dependes on telemetry,
#       and telemetry depends on common.cpp in utils folder.
#       I couldn't bind these to into a single target.
add_library_bundle(address_utils SOURCES address_utils.cpp PUBLIC_BUNDLE telemetry types)

add_library_bundle(utils SOURCES common.cpp error.cpp host_utils.cpp io.cpp misc.cpp xml_parser.cpp random.cpp INTERFACE boost_headers)

# I'm worring about Boost::url. It would be included here and entrypoint also.
# Let's see what happens.
add_library_bundle(
    strings_utils
    SOURCES
    strings.cpp
    PRIVATE
    Boost::url)

