add_library(uh_crypto STATIC scrypt.cpp hmac.cpp hash.cpp ossl_base.cpp)
target_link_libraries(
    uh_crypto
    PRIVATE uh_utils
    PUBLIC OpenSSL::Crypto)
target_precompile_headers(uh_crypto REUSE_FROM uh_precompile_headers)
