add_static_and_shared(
    uh_license
    SOURCES
    license.cpp
    PRIVATE
    OpenSSL::Crypto
    PUBLIC
    uh_utils)
