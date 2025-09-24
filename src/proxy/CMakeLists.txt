find_package(OpenSSL REQUIRED)
add_library(proxy service.cpp request_factory.cpp)
target_link_libraries(proxy types utils network entrypoint OpenSSL::SSL OpenSSL::Crypto)
