add_library(uh_common_crypto hash.cpp hmac.cpp)
target_link_libraries(uh_common_crypto uh_common_utils)
