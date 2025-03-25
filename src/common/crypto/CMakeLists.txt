find_package(OpenSSL REQUIRED)

add_library_bundle(uhcrypto
    SOURCES hash.cpp hmac.cpp ossl_base.cpp scrypt.cpp
    PUBLIC OpenSSL::Crypto)
