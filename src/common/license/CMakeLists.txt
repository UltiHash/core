find_package(OpenSSL REQUIRED)

add_library(license license.cpp usage.cpp internal/util.cpp)
target_link_libraries(license OpenSSL::Crypto db network nlohmann_json etcd-cpp-api)
