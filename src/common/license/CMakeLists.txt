add_library(uh_license license.cpp)
target_link_libraries(
    uh_license
    PRIVATE OpenSSL::Crypto
    PUBLIC uh_utils)
