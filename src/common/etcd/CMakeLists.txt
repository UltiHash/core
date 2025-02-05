add_module(
    etcd
    SOURCES
    utils.cpp
    INTERFACE
    etcd-cpp-api
    PUBLIC_MODULE
    telemetry)

include(CMakePrintHelpers)
cmake_print_properties(TARGETS zpp_bits PROPERTIES TYPE)
