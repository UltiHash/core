add_library(uh_license STATIC license.cpp)
target_link_libraries(
    uh_license
    PRIVATE OpenSSL::Crypto
    PUBLIC uh_utils)
target_precompile_headers(uh_license REUSE_FROM uh_precompile_headers)
