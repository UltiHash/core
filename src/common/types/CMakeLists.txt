add_library(uh_types STATIC address.cpp)
target_link_libraries(uh_types PUBLIC zpp_bits)
target_precompile_headers(
    uh_types
    PUBLIC
    <boost/asio.hpp>
    <chrono>
    <zpp_bits.h>
    <functional>
    <iomanip>)
