#ifndef REST_NODE_SRC_S3_AUTHENTICATOR
#define REST_NODE_SRC_S3_AUTHENTICATOR

#include "s3_parser.h"

namespace uh::cluster {

//------------------------------------------------------------------------------

    class s3_authenticator
    {
    private:
        parsed_request_wrapper& m_parsed_request;
        const http::request_parser<http::string_body>& m_received_request;

        std::string m_access_key {};
        std::set<std::string> m_signed_headers {};
        std::string m_signature {};

    public:
        s3_authenticator(const http::request_parser<http::string_body>& received_request, parsed_request_wrapper& parsed_request);

        std::string
        get_canonical_uri() const;

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
