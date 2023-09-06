#ifndef REST_NODE_SRC_S3_AUTHENTICATOR
#define REST_NODE_SRC_S3_AUTHENTICATOR

#include "s3_parser.h"

namespace uh::cluster {

//------------------------------------------------------------------------------

    class s3_authenticator
    {
    private:
        const parsed_request_wrapper& m_parsed_request;
        const http::request<http::string_body>& m_received_request;

    public:
        explicit s3_authenticator(const http::request<http::string_body>& received_request, const parsed_request_wrapper& parsed_request);

        std::string
        get_canonical_uri();

        std::string
        get_canonical_query_string();

        std::string
        get_headers();

        std::string
        get_canonical_request();

        std::string
        get_string_to_sign();

        void
        authenticate();

    };

//------------------------------------------------------------------------------

} // namespace uh::cluster

#endif
