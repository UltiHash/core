add_library(uh_common_types address.cpp)
target_link_libraries(uh_common_types ${Boost_LIBRARIES} zpp_bits)
