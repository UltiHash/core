#include "s3_parser.h"
#include <iostream>

namespace uh::cluster {

//------------------------------------------------------------------------------

    const std::unordered_map <s3_req_type, std::set<s3_fields>> s3_parser::static_s3_valid_fields =
            {
                    { s3_req_type::put_object, { x_amz_acl, x_amz_grant_full_control, x_amz_grant_read, x_amz_grant_read_acp,
                                                 x_amz_grant_write_acp, x_amz_storage_class, x_amz_request_payer, x_amz_tagging,
                                                 x_amz_expected_bucket_owner, x_amz_meta_author, x_amz_content_sha256 } },
                    { s3_req_type::get_object, { x_amz_request_payer, x_amz_expected_bucket_owner, x_amz_content_sha256 } }, //TODO: put the common x-amz-header outside this enum
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
                        {"x-amz-content-sha256", x_amz_content_sha256},

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

    // make the map in lowercase
    http_fields http_field_to_enum (const std::string &field)
    {
        static const std::unordered_map<std::string, http_fields> enum_map =
                {
                        {"host", http_fields::host},
                        {"user-agent", http_fields::user_agent},
                        {"accept" , http_fields::http_accept},
                        {"connection", http_fields::connection},
                        {"server", http_fields::server},
                        {"put", http_fields::put},
                        {"authorization", http_fields::authorization},
                        {"expect", http_fields::expect},
                        {"cache-control", http_fields::cache_control},
                        {"content-disposition", http_fields::content_disposition},
                        {"content-encoding", http_fields::content_encoding},
                        {"content-language", http_fields::content_language},
                        {"content-length", http_fields::content_length},
                        {"content-md5", http_fields::content_md5},
                        {"content-type", http_fields::content_type},
                        {"expires", http_fields::expires},
                        {"get", http_fields::get},
                        {"if-match", http_fields::if_match},
                        {"if-modified-since", http_fields::if_modified_since},
                        {"if-none-match", http_fields::if_none_match},
                        {"if-unmodified-since", http_fields::if_unmodified_since},
                        {"range", http_fields::range},
                        {"delete", http_fields::delete_},
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
                std::string recev_header = header.name_string();
                std::transform(recev_header.begin(), recev_header.end(), recev_header.begin(), [](unsigned char c)
                {
                    return std::tolower(c);
                });
                std::cout << recev_header << std::endl;
                m_parsed_req_wrapper.http_parsed_fields.emplace(http_field_to_enum(recev_header), header.value());
            }
        }

        std::string verb_string = to_string(m_recv_req.get().base().method());
        std::transform(verb_string.begin(), verb_string.end(), verb_string.begin(), [](unsigned char c)
        {
            return std::tolower(c);
        });
        m_parsed_req_wrapper.verb = http_field_to_enum(verb_string);
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
