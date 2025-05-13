find_package(OpenSSL REQUIRED)

add_library(http_server http_server.cpp)
target_link_libraries(http_server PRIVATE httplib ${OPENSSL_LIBRARIES}
                                          Boost::filesystem cpr::cpr magic_enum)
target_include_directories(http_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
