add_library(ep_http
            stream.cpp
            range.cpp
            raw_body.cpp
            string_body.cpp
            response.cpp
            chunked_body.cpp
            basic_auth.cpp
            raw_request.cpp
            aws4_hmac_sha256.cpp
            no_auth.cpp
            request_factory.cpp
            chunk_body_sha256.cpp
            request.cpp
            command_exception.cpp
            raw_body_sha256.cpp)
target_link_libraries(ep_http types utils entrypoint)
