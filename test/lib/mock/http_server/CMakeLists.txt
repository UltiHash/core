find_package(OpenSSL REQUIRED)

add_library(http_server SHARED http_server.cpp)
target_link_libraries(http_server PRIVATE httplib ${OPENSSL_LIBRARIES}
                                          Boost::filesystem cpr::cpr)
