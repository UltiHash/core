find_package(OpenSSL REQUIRED)

add_library(uhcrypto
            hash.cpp
            hmac.cpp
            ossl_base.cpp
            scrypt.cpp)

target_link_libraries(uhcrypto OpenSSL::Crypto)
