#include "s3_parser.h"
#include <iostream>

namespace uh::cluster {

//------------------------------------------------------------------------------

    const std::unordered_map <s3_req_type, std::set<s3_fields>> s3_parser::static_s3_valid_fields =
            {
                    { s3_req_type::put_object, { x_amz_acl, x_amz_grant_full_control, x_amz_grant_read, x_amz_grant_read_acp,
                                                 x_amz_grant_write_acp, x_amz_storage_class, x_amz_request_payer, x_amz_tagging,
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
                    { s3_req_type::put_object, { http_fields::content_disposition, http_fields::content_encoding, http_fields::content_language, http_fields::content_length, http_fields::content_md5, http_fields::content_type, http_fields::expires } },
                    { s3_req_type::get_object, { http_fields::if_match, http_fields::if_modified_since, http_fields::if_none_match, http_fields::if_unmodified_since, http_fields::range } },
                    { s3_req_type::get_object, { http_fields::if_match, http_fields::if_modified_since, http_fields::if_none_match, http_fields::if_unmodified_since, http_fields::range } },
                    { s3_req_type::copy_object, { http_fields::content_disposition, http_fields::content_encoding, http_fields::content_language, http_fields::content_type, http_fields::expires } },
                    { s3_req_type::delete_object, {  } },
                    { s3_req_type::create_bucket, {  } },
                    { s3_req_type::delete_bucket, {  } },
            };

//------------------------------------------------------------------------------

    s3_fields s3_field_to_enum (const std::string &field)
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

    http_fields http_field_to_enum (const std::string &field)
    {
        static const std::unordered_map<std::string, http_fields> enum_map =
                {
                        {"Host", http_fields::host},
                        {"User-Agent", http_fields::user_agent},
                        {"Accept" , http_fields::http_accept},
                        {"Connection", http_fields::connection},
                        {"Server", http_fields::server},
                        {"PUT", http_fields::put},
                        {"Authorization", http_fields::authorization},
                        {"Expect", http_fields::expect},
                        {"Cache-Control", http_fields::cache_control},
                        {"Content-Disposition", http_fields::content_disposition},
                        {"Content-Encoding", http_fields::content_encoding},
                        {"Content-Language", http_fields::content_language},
                        {"Content-Length", http_fields::content_length},
                        {"Content-MD5", http_fields::content_md5},
                        {"Content-Type", http_fields::content_type},
                        {"Expires", http_fields::expires},
                        {"GET", http_fields::get},
                        {"If-Match", http_fields::if_match},
                        {"If-Modified-Since", http_fields::if_modified_since},
                        {"If-None-Match", http_fields::if_none_match},
                        {"If-Unmodified-Since", http_fields::if_unmodified_since},
                        {"Range", http_fields::range},
                        {"DELETE", http_fields::delete_},
                };

        auto it = enum_map.find(field);
        if (it != enum_map.end())
        {
            return it->second;
        }
        else
        {
            return http_fields::not_known;
        }
    };

//------------------------------------------------------------------------------

    s3_parser::s3_parser(const http::request_parser<http::string_body>& recv_req) : m_recv_req(recv_req), m_s3_vfields(static_s3_valid_fields),
    m_http_vfields(static_http_valid_fields)
    {}

//------------------------------------------------------------------------------

    parsed_request_wrapper&
    s3_parser::parse()
    {
        if (m_recv_req.get().base().version() != 11)
        {
            throw std::runtime_error("bad http version. support exists only for HTTP 1.1.\n");
        }

        for (const auto& header : m_recv_req.get())
        {
            if (header.name_string().starts_with("x-"))
            {
                m_parsed_req_wrapper.s3_parsed_fields.emplace(s3_field_to_enum(header.name_string()), header.value());
            }
            else
            {
                m_parsed_req_wrapper.http_parsed_fields.emplace(http_field_to_enum(header.name_string()), header.value());
            }
        }

        m_parsed_req_wrapper.verb = http_field_to_enum(to_string(m_recv_req.get().base().method()));
        m_target = m_recv_req.get().base().target();

        auto index = m_target.find('?');
        if ( index != std::string::npos)
            m_parsed_req_wrapper.object_key = m_target.substr(1, index);
        else
            m_parsed_req_wrapper.object_key = m_target.substr(1);

        m_parsed_req_wrapper.bucket_id = m_parsed_req_wrapper.http_parsed_fields[http_fields::host].
                substr(0, m_parsed_req_wrapper.http_parsed_fields[http_fields::host].find(':'));

        m_parsed_req_wrapper.req_type = get_type();
        m_parsed_req_wrapper.body = m_recv_req.get().body();

        sanitizer();

        return m_parsed_req_wrapper;
    }

//------------------------------------------------------------------------------

    s3_req_type
    s3_parser::get_type() const
    {
        switch (m_parsed_req_wrapper.verb)
        {
            case http_fields::put:
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
            case http_fields::get:
                if (!m_target.empty() && (m_target.find('?') == std::string::npos))
                {
                    return get_object;
                }
            case http_fields::delete_:
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

//------------------------------------------------------------------------------

    void
    s3_parser::sanitizer()
    {
        for (const auto& tag: m_parsed_req_wrapper.http_parsed_fields)
        {
            if (!m_http_common_fields.contains(tag.first))
                if (!m_http_vfields.at(m_parsed_req_wrapper.req_type).contains(tag.first))
                    throw std::runtime_error("encountered invalid field for the given request.\n");
        }

        // check for s3_fields
        for (const auto& tag: m_parsed_req_wrapper.s3_parsed_fields)
        {
            if (!m_s3_vfields.at(m_parsed_req_wrapper.req_type).contains(tag.first))
                throw std::runtime_error("encountered invalid field for the given request.\n");
        }
    }

//------------------------------------------------------------------------------

} // uh::cluster
