add_library(uh_http raw_body.cpp chunk_body_sha256.cpp command_exception.cpp basic_auth.cpp raw_body_sha256.cpp request.cpp string_body.cpp aws4_hmac_sha256.cpp range.cpp response.cpp no_auth.cpp beast_utils.cpp request_factory.cpp chunked_body.cpp)
target_link_libraries(uh_http
PRIVATE
uh_telemetry
uh_entrypoint
PUBLIC
uh_user
uh_types
uh_utils
uh_crypto
uh_coroutines
Boost::url
# boost::asio
)
