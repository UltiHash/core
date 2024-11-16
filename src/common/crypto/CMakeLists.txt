add_library(uh_crypto STATIC scrypt.cpp hmac.cpp hash.cpp ossl_base.cpp)
target_link_libraries(uh_crypto uh_utils OpenSSL::Crypto)
