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
        std::string m_date {};
        std::string m_region {};
        std::string m_service {};

        std::string m_secret_key = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY";

        std::set<std::string> m_signed_headers {};
        std::string m_signature {};

    public:
        s3_authenticator(const http::request_parser<http::string_body>& received_request, parsed_request_wrapper& parsed_request);

        [[nodiscard]] std::string
        get_canonical_uri() const;

        [[nodiscard]] std::string
        get_canonical_query_string() const;

        [[nodiscard]] std::string
        get_headers() const;

        [[nodiscard]] std::string
        get_canonical_request() const;

        [[nodiscard]] std::string
        get_string_to_sign() const;

        [[nodiscard]] std::string
        get_formatted_date() const;

        [[nodiscard]] std::string
        get_scope() const;

        [[nodiscard]] std::string
        signing_key() const;

        [[nodiscard]] std::string
        ISO_8601_timestamp() const;

        [[nodiscard]] std::string
        hmac_sha_256(const std::string& payload) const;

        [[nodiscard]] std::string
        hmac_sha_256(const std::string& payload, const std::string& signing_key) const;

        void
        authenticate();

    };

//------------------------------------------------------------------------------

} // namespace uh::cluster

#endif
