add_library(uh_precompile_headers STATIC pch.cpp)
target_include_directories(uh_precompile_headers
                           PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(uh_precompile_headers
                      PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES})
target_precompile_headers(
    uh_precompile_headers
    PRIVATE
    <atomic>
    <chrono>
    <functional>
    <iomanip>
    <map>
    <ostream>
    <source_location>
    <sstream>
    <string>
    <thread>
    <typeinfo>
    <vector>
    <CLI/CLI.hpp>
    <boost/algorithm/string.hpp>
    <boost/asio.hpp>
    <boost/beast.hpp>
    <boost/log/trivial.hpp>
    <boost/property_tree/ptree.hpp>
    <boost/url.hpp>
    <magic_enum/magic_enum.hpp>
    <openssl/bio.h>
    <openssl/core_names.h>
    <openssl/err.h>
    <openssl/evp.h>
    <openssl/kdf.h>
    <openssl/params.h>
    <openssl/pem.h>
    <nlohmann/json.hpp>
    <zpp_bits.h>)
