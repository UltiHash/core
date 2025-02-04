add_module(
    types
    SOURCES
    address.cpp
    INTERFACE
    project_includes
    boost_headers
    zpp_bits)
# add_library(types SHARED address.cpp) target_link_libraries(types PUBLIC
# project_includes boost_headers zpp_bits) target_precompile_headers(types
# REUSE_FROM uh_precompile_headers)
