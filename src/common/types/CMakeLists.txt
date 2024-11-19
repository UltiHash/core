add_library(uh_types STATIC address.cpp)
target_link_libraries(uh_types PUBLIC zpp_bits)
# target_precompile_headers(uh_types PUBLIC big_int.h common_types.h)
