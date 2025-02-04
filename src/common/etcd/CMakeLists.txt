add_library(etcd SHARED utils.cpp)
target_link_libraries(etcd PUBLIC project_includes etcd-cpp-api telemetry)
# target_precompile_headers(etcd REUSE_FROM uh_precompile_headers)
