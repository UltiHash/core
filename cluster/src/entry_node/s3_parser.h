#ifndef REST_NODE_SRC_S3_PARSER
#define REST_NODE_SRC_S3_PARSER

#include "logging/logging_boost.h"
#include <set>

namespace uh::cluster {

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

    auto s3_field_to_enum = [](const std::string &field)
    {
        static const std::unordered_map<std::string, s3_fields> enum_map =
                {
                        {"x-amz-acl",           x_amz_acl},
                        {"x-amz-grant-full-control",    x_amz_grant_full_control},
                        {"x-amz-grant-read",    x_amz_grant_read},
                        {"x-amz-grant-read-acp",    x_amz_grant_read_acp},
                        {"x-amz-grant-write",    x_amz_grant_write},
                        {"x-amz-grant-write-acp",   x_amz_grant_write_acp},
                        {"x-amz-object-ownership", x_amz_object_ownership},
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

    auto http_field_to_enum = [](const std::string &field)
    {
        static const std::unordered_map<std::string, http_fields> enum_map =
                {
                        {"Host", host},
                        {"User-Agent", user_agent},
                        {"Accept" , http_accept},
                        {"Connection", connection},
                        {"Server", server},
                        {"PUT", put},
                        {"Cache-Control", cache_control},
                        {"Content-Disposition", content_disposition},
                        {"Content-Encoding", content_encoding},
                        {"Content-Language", content_language},
                        {"Content-Length", content_length},
                        {"Content-MD5", content_md5},
                        {"Content-Type", content_type},
                        {"Expires", expires},
                        {"GET", get},
                        {"If-Match", if_match},
                        {"If-Modified-Since", if_modified_since},
                        {"If-None-Match", if_none_match},
                        {"If-Unmodified-Since", if_unmodified_since},
                        {"Range", range},
                        {"DELETE", delete_},
                };

        auto it = enum_map.find(field);
        if (it != enum_map.end())
        {
            return it->second;
        }
        else
        {
            return not_known;
        }
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

    class s3_parser
    {
    private:
        static const std::unordered_map <s3_req_type, std::set<s3_fields>> static_s3_valid_fields;
        static const std::unordered_map <s3_req_type, std::set<http_fields>> static_http_valid_fields;
        /* static checks for initializing everytime we call it, so we access the data through a
         * class member variable */
        const std::unordered_map <s3_req_type, std::set<s3_fields>>& m_s3_vfields;
        const std::unordered_map <s3_req_type, std::set<http_fields>>& m_http_vfields;
        const std::set<http_fields> m_http_common_fields = {host, user_agent, http_accept, connection, date, server};

        const http::request<http::string_body>& m_recv_req;
        http::verb m_verb {};
        std::string_view m_target;

        struct parsed_request_wrapper
        {
            std::unordered_map <s3_fields, std::string_view> s3_parsed_fields;
            std::unordered_map <http_fields, std::string_view> http_parsed_fields;
            s3_req_type req_type = not_initialized;
            std::string bucket_id;
            std::string object_key;
            std::string_view body;
        };
        parsed_request_wrapper m_parsed_req_wrapper;

    public:
        explicit s3_parser
        (const http::request<http::string_body>& recv_req) : m_recv_req(recv_req), m_s3_vfields(static_s3_valid_fields),
        m_http_vfields(static_http_valid_fields)
        {}

        const
        parsed_request_wrapper& parse()
        {
            if (m_recv_req.base().version() != 11)
            {
                throw std::runtime_error("bad http version. support exists only for HTTP 1.1.\n");
            }

            m_verb = m_recv_req.base().method();
            m_target = m_recv_req.base().target();

            for (const auto& header : m_recv_req)
            {
                if (header.name_string().starts_with("x-"))
                    m_parsed_req_wrapper.s3_parsed_fields.emplace(s3_field_to_enum(header.name_string()), header.value());
                else
                    m_parsed_req_wrapper.http_parsed_fields.emplace(http_field_to_enum(header.name_string()), header.value());
            }

            m_parsed_req_wrapper.req_type = get_type();

            sanitizer();

            m_parsed_req_wrapper.body = m_recv_req.body();
            return m_parsed_req_wrapper;
        }

        s3_req_type
        get_type()
        {
            switch (m_verb)
            {
                case http::verb::put:
                    if (m_target == "/")
                    {
                        return create_bucket;
                    }
                    else if (m_parsed_req_wrapper.s3_parsed_fields.contains(x_amz_copy_source))
                    {
                        return copy_object;
                    }
                    else if (!m_target.empty() && (m_target.find('?') == std::string::npos))
                    {
                        return put_object;
                    }
                case http::verb::get:
                    if (!m_target.empty() && (m_target.find('?') == std::string::npos))
                    {
                        return get_object;
                    }
                case http::verb::delete_:
                    if (m_target == "/")
                    {
                        return delete_bucket;
                    }
                    else if (!m_target.empty() && (m_target.find('?') == std::string::npos))
                    {
                        return delete_object;
                    }
                default:
                    throw std::runtime_error("bad http verb.");
            }
        }

        void sanitizer()
        {
            // check for common fields

            // check for s3_fields
            for (const auto& tag: m_parsed_req_wrapper.s3_parsed_fields)
            {
                if (!m_s3_vfields.at(m_parsed_req_wrapper.req_type).contains(tag.first))
                    throw std::runtime_error("encountered invalid field for the given request.\n");
            }

            for (const auto& tag: m_parsed_req_wrapper.http_parsed_fields)
            {
                if (!m_http_common_fields.contains(tag.first))
                    if (!m_http_vfields.at(m_parsed_req_wrapper.req_type).contains(tag.first))
                        throw std::runtime_error("encountered invalid field for the given request.\n");
            }
        }

    };

//------------------------------------------------------------------------------

    const std::unordered_map <s3_req_type, std::set<s3_fields>> s3_parser::static_s3_valid_fields =
        {
            { s3_req_type::put_object, { x_amz_acl, x_amz_grant_full_control, x_amz_grant_read,
                                         x_amz_grant_read_acp, x_amz_grant_write_acp,
                                         x_amz_storage_class, x_amz_request_payer, x_amz_tagging,
                                         x_amz_expected_bucket_owner, x_amz_meta_author } },
            { s3_req_type::get_object, { x_amz_request_payer, x_amz_expected_bucket_owner } },
            { s3_req_type::copy_object, { x_amz_acl, x_amz_copy_source, x_amz_copy_source_if_match, x_amz_copy_source_if_modified_since,
                                          x_amz_copy_source_if_none_match, x_amz_copy_source_if_unmodified_since, x_amz_grant_full_control, x_amz_grant_read,
                                        x_amz_grant_read_acp, x_amz_grant_write_acp, x_amz_metadata_directive, x_amz_tagging_directive,
                                        x_amz_storage_class, x_amz_request_payer, x_amz_tagging, x_amz_expected_bucket_owner, x_amz_source_expected_bucket_owner } },
            { s3_req_type::delete_object, { x_amz_request_payer, x_amz_expected_bucket_owner } },
            {s3_req_type::create_bucket, { x_amz_acl, x_amz_grant_full_control, x_amz_grant_read, x_amz_grant_read_acp, x_amz_grant_write, x_amz_grant_write_acp, x_amz_object_ownership }},
            { s3_req_type::delete_bucket, { x_amz_expected_bucket_owner } },
        };

//------------------------------------------------------------------------------

    const std::unordered_map <s3_req_type, std::set<http_fields>> s3_parser::static_http_valid_fields =
        {
            { s3_req_type::put_object, { content_disposition, content_encoding, content_language, content_length, content_md5, content_type, expires } },
            { s3_req_type::get_object, { if_match, if_modified_since, if_none_match, if_unmodified_since, range } },
            { s3_req_type::get_object, { if_match, if_modified_since, if_none_match, if_unmodified_since, range } },
            { s3_req_type::copy_object, { content_disposition, content_encoding, content_language, content_type, expires } },
            { s3_req_type::delete_object, {  } },
            { s3_req_type::create_bucket, {  } },
            { s3_req_type::delete_bucket, {  } },
        };

//------------------------------------------------------------------------------

} // namespace uh::cluster

#endif // REST_NODE_SRC_S3_PARSER
