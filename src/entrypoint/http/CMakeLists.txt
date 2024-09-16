add_library(uh_http raw_body.cpp string_body.cpp response.cpp chunked_body.cpp default_request_factory.cpp beast_utils.cpp request_factory.cpp auth_utils.cpp chunk_body_sha256.cpp request.cpp command_exception.cpp auth_request_factory.cpp raw_body_sha256.cpp)
target_link_libraries(uh_http uh_telemetry uh_utils uh_crypto uh_entrypoint uh_types uh_user)
