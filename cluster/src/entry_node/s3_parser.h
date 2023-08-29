#ifndef REST_NODE_SRC_S3_PARSER
#define REST_NODE_SRC_S3_PARSER

#include "logging/logging_boost.h"

namespace uh::cluster {

//------------------------------------------------------------------------------

    enum s3_fields
    {
        unknown = 0,
        put,
        get,
        host,
        user_agent,
        http_accept,
        connection,
        content_type,
        content_length,
        bucket_id,
        object_key,
        x_amz_acl,
        x_amz_grant_full_control,
        x_amz_grant_read,
        x_amz_grant_read_acp,
        x_amz_grant_write_acp,
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

    auto s3_field_to_enum = [](const std::string &field)
    {
        static const std::unordered_map<std::string, s3_fields> enum_map =
                {
                        {"PUT", put},
                        {"HOST", host},
                        {"User-Agent", user_agent},
                        {"Accept" , http_accept},
                        {"Connection", connection},
                        {"Content-Type", content_type},
                        {"Content-Length", content_length},
                        {"GET", get},
                        {"x-amz-acl",           x_amz_acl},
                        {"x-amz-grant-full-control",    x_amz_grant_full_control},
                        {"x-amz-grant-read",    x_amz_grant_read},
                        {"x-amz-grant-read-acp",    x_amz_grant_read_acp},
                        {"x-amz-grant-write-acp",   x_amz_grant_write_acp},
                        {"x-amz-storage-class", x_amz_storage_class},
                        {"x-amz-request-payer", x_amz_request_payer},
                        {"x-amz-tagging",       x_amz_tagging},
                        {"x-amz-meta-author",           x_amz_meta_author},
                        {"x-amz-expected-bucket-owner", x_amz_expected_bucket_owner},
                        {"x-amz-copy-source",   x_amz_copy_source},
                        {"x-amz-metadata-directive",   x_amz_metadata_directive},
                        {"x-amz-tagging-directive",   x_amz_tagging_directive},

                };

        auto it = enum_map.find(field);
        if (it != enum_map.end())
        {
            return it->second;
        }
        else
        {
            return unknown;
        }
    };

//------------------------------------------------------------------------------

    enum s3_req_type
    {
        not_initialized = 0,
        put_object,
        get_object,
        copy_object,
    };

//------------------------------------------------------------------------------

    class s3_parser
    {
    private:
        const http::request<http::string_body>& m_recv_req;
        http::verb verb;
        std::string_view target;

        struct parsed_request_wrapper
        {
            std::unordered_map <s3_fields, std::string_view> parsed_request;
            s3_req_type m_req_type = not_initialized;
            std::string bucket;
            std::string object_key;
            std::string_view body;
        };
        parsed_request_wrapper m_parsed_req_wrapper;

    public:
        explicit s3_parser
        (const http::request<http::string_body>& recv_req) : m_recv_req(recv_req)
        {}

        void parse()
        {
            if (m_recv_req.base().version() != 11)
            {
                throw std::runtime_error("Bad HTTP version. Support exists for only HTTP 1.1.");
            }

            verb = m_recv_req.base().method();
            target = m_recv_req.base().target();
            for (const auto& header : m_recv_req)
            {
                m_parsed_req_wrapper.parsed_request.emplace(s3_field_to_enum(header.name_string()), header.value());
                std::cout << header.name_string() << " " << header.value() << std::endl;
            }

//            m_req_type = get_type();
        }

        s3_req_type get_type() const
        {
            switch (verb)
            {
                case http::verb::put:
                    break;
                case http::verb::get:
                    break;

            }
        }

    };

//------------------------------------------------------------------------------

} // namespace uh::cluster

#endif // REST_NODE_SRC_S3_PARSER
