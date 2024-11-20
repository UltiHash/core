add_library(uh_user STATIC db.cpp)
target_link_libraries(
    uh_user
    PRIVATE uh_http uh_utils
    PUBLIC uh_crypto uh_types uh_db uh_policy)
target_precompile_headers(uh_user REUSE_FROM uh_precompile_headers)
