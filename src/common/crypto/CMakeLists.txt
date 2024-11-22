add_static_and_shared(uh_crypto SOURCES scrypt.cpp hmac.cpp hash.cpp ossl_base.cpp
    PRIVATE uh_utils
    PUBLIC OpenSSL::Crypto)
