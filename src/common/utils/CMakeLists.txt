add_library(common_utils SHARED common.cpp)
target_link_libraries(common_utils PUBLIC project_includes)
# target_precompile_headers(common_utils REUSE_FROM uh_precompile_headers)

add_library(address_utils SHARED address_utils.cpp)
target_include_directories(address_utils PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(address_utils PUBLIC project_includes telemetry types)
# target_precompile_headers(address_utils REUSE_FROM uh_precompile_headers)

# error.cpp host_utils.cpp io.cpp misc.cpp random.cpp strings.cpp
# temp_directory.cpp xml_parser.cpp
