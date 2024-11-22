add_static_and_shared(uh_types SOURCES address.cpp PUBLIC zpp_bits)

include(CMakePrintHelpers)
cmake_print_properties(TARGETS uh_types PROPERTIES INTERFACE_LINK_LIBRARIES)
