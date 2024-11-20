add_library(uh_aws STATIC arn.cpp)
target_link_libraries(uh_aws PRIVATE uh_utils)
target_precompile_headers(uh_aws REUSE_FROM uh_precompile_headers)
