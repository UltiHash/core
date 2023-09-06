#ifndef REST_NODE_SRC_S3_PARSER
#define REST_NODE_SRC_S3_PARSER

#include "logging/logging_boost.h"
#include <set>
#include <boost/beast/http.hpp>


namespace uh::rest {

//------------------------------------------------------------------------------

    namespace http = boost::beast::http;

//------------------------------------------------------------------------------

    enum s3_fields : uint8_t
    {
        unknown = 0,
        bucket_id,
        object_key,
        x_amz_acl,
        x_amz_grant_full_control,
        x_amz_grant_read,
        x_amz_grant_read_acp,
        x_amz_grant_write,
        x_amz_grant_write_acp,
        x_amz_object_ownership,
        x_amz_storage_class,
        x_amz_tagging,
        x_amz_meta_author,
        x_amz_request_payer,
        x_amz_copy_source,
        x_amz_copy_source_if_match,
        x_amz_copy_source_if_modified_since,
        x_amz_copy_source_if_none_match,
        x_amz_copy_source_if_unmodified_since,
        x_amz_metadata_directive,
        x_amz_tagging_directive,
        x_amz_expected_bucket_owner,
        x_amz_source_expected_bucket_owner,
    };

//------------------------------------------------------------------------------

    enum http_fields : uint8_t
    {
        not_known = 0,
        host,
        authorization,
        user_agent,
        http_accept,
        connection,
        date,
        server,
        put,
        get,
        delete_,
        cache_control,
        content_disposition,
        content_encoding,
        content_language,
        content_length,
        content_md5,
        content_type,
        expires,
        range,
        if_match,
        if_modified_since,
        if_none_match,
        if_unmodified_since
    };

//------------------------------------------------------------------------------

    enum s3_req_type
    {
        not_initialized = 0,
        put_object,
        get_object,
        get_object_information,
        copy_object,
        delete_object,
        create_bucket,
        delete_bucket,
    };

//------------------------------------------------------------------------------


    struct parsed_request_wrapper
    {
        enum http_fields verb;
        std::unordered_multimap <s3_fields, std::string_view> s3_parsed_fields;
        std::unordered_map <http_fields, std::string_view> http_parsed_fields;
        s3_req_type req_type = not_initialized;
        std::string bucket_id;
        std::string object_key;
        std::string_view body;
    };

//------------------------------------------------------------------------------

    class s3_parser
    {
    private:
        static const std::unordered_map <s3_req_type, std::set<s3_fields>> static_s3_valid_fields;
        static const std::unordered_map <s3_req_type, std::set<http_fields>> static_http_valid_fields;
        /* static checks for initializing everytime we call it, so we access the data through a
         * class member variable */
        const std::unordered_map <s3_req_type, std::set<s3_fields>>& m_s3_vfields;
        const std::unordered_map <s3_req_type, std::set<http_fields>>& m_http_vfields;
        const std::set<http_fields> m_http_common_fields = {host, user_agent, http_accept, connection, date, server, authorization};

        const http::request<http::string_body>& m_recv_req;
        parsed_request_wrapper m_parsed_req_wrapper;
        std::string_view m_target;

    public:
        explicit s3_parser
        (const http::request<http::string_body>& recv_req);

        const
        parsed_request_wrapper& parse();

        s3_req_type
        get_type() const;

        void
        sanitizer();

    };

//------------------------------------------------------------------------------

} // namespace uh::rest

#endif // REST_NODE_SRC_S3_PARSER
