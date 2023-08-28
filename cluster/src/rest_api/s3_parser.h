#ifndef REST_NODE_SRC_S3_PARSER
#define REST_NODE_SRC_S3_PARSER

#include <boost/beast/http/basic_parser.hpp>
#include "logging/logging_boost.h"
#include <set>

namespace uh::rest {

//------------------------------------------------------------------------------

    enum req_types
    {
        put_object = 0,
        copy_object,
        get_object,
    };

//------------------------------------------------------------------------------

    enum http_fields
    {
        host = 0,
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

    enum s3_fields
    {
        unknown = 0,
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

    s3_fields
    s3_field_to_enum(const std::string &field)
    {
        static const std::unordered_map<std::string, s3_fields> enum_map =
                {
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
        } else
        {
            return unknown;
        }
    }

//------------------------------------------------------------------------------

    struct http_fields_object
    {
        std::string host;
        std::string cache_control;
        std::string content_disposition;
        std::string content_encoding;
        std::string content_language;
        std::string content_length;
        std::string content_md5;
        std::string content_type;
        std::string expires;
        std::string range;
        std::string if_match;
        std::string if_modified_since;
        std::string if_none_match;
        std::string if_unmodified_since;
    };

//------------------------------------------------------------------------------

    struct s3_copy_object
    {
        std::string x_amz_copy_source;
        std::string x_amz_copy_source_if_match;
        std::string x_amz_copy_source_if_modified_since;
        std::string x_amz_copy_source_if_none_match;
        std::string x_amz_copy_source_if_unmodified_since;
        std::string x_amz_metadata_directive;
        std::string x_amz_tagging_directive;
    };

//------------------------------------------------------------------------------

    struct s3_put
    {
        std::string x_amz_acl;
        std::string x_amz_grant_full_control;
        std::string x_amz_grant_read;
        std::string x_amz_grant_read_acp;
        std::string x_amz_grant_write_acp;
        std::string x_amz_storage_class;
        std::string x_amz_tagging;

        std::unique_ptr<s3_copy_object> copy_object {};
    };

//------------------------------------------------------------------------------

    struct s3_get
    {
    };

//------------------------------------------------------------------------------

    struct s3_req_object
    {
        enum req_types req_type;
        http_fields_object http_headers;
        std::string bucket_id;
        std::string object_key;
        // TODO: don't copy the body
        std::stringstream body_stream;

        std::unique_ptr<s3_put> put {};
        std::unique_ptr<s3_get> get {};

        std::string x_amz_request_payer;
        std::string x_amz_expected_bucket_owner;
    };

//------------------------------------------------------------------------------

    using namespace boost::beast;
    using namespace boost::beast::http;
    template<bool isRequest>
    class s3_parser : public basic_parser<isRequest> {
    private:

        /** Called after receiving the request-line.

            This virtual function is invoked after receiving a request-line
            when parsing HTTP requests.
            It can only be called when `isRequest == true`.

            @param method The verb enumeration. If the method string is not
            one of the predefined strings, this value will be @ref verb::unknown.

            @param method_str The unmodified string representing the verb.

            @param target The request-target.

            @param version The HTTP-version. This will be 10 for HTTP/1.0,
            and 11 for HTTP/1.1.

            @param ec An output parameter which the function may set to indicate
            an error. The error will be clear before this function is invoked.
        */
        void
        on_request_impl(
                verb method,                // The method verb, verb::unknown if no match
                string_view method_str,     // The method as a string
                string_view target,         // The request-target
                int version,                // The HTTP-version
                error_code &ec) override
        {
            if (version != 11)
            {
                ec = make_error_code(http::error::bad_version);
            }
            else
            {
                switch (method)
                {
                    case verb::put:
                        if (!target.empty() && (target.find('?') == std::string::npos))
                        {
                            m_parsed_struct.object_key = target.substr(1);
                            m_parsed_struct.req_type = put_object;
                        }
                        else
                        {
                            ec = make_error_code(http::error::bad_target);
                        }
                        break;

                    case verb::get:
                        if (!target.empty() && (target.find('?') == std::string::npos))
                        {
                            m_parsed_struct.object_key = target.substr(1);
                            m_parsed_struct.req_type = get_object;
                        }
                        else
                        {
                            ec = make_error_code(http::error::bad_target);
                        }
                        break;

                    default:
                        ec = make_error_code(http::error::bad_method);
                        break;
                }
            }

        }   // The error returned to the caller, if any

        /** Called after receiving the status-line.

            This virtual function is invoked after receiving a status-line
            when parsing HTTP responses.
            It can only be called when `isRequest == false`.

            @param code The numeric status code.

            @param reason The reason-phrase. Note that this value is
            now obsolete, and only provided for historical or diagnostic
            purposes.

            @param version The HTTP-version. This will be 10 for HTTP/1.0,
            and 11 for HTTP/1.1.

            @param ec An output parameter which the function may set to indicate
            an error. The error will be clear before this function is invoked.
        */
        void
        on_response_impl(
                int code,                   // The status-code
                string_view reason,         // The obsolete reason-phrase
                int version,                // The HTTP-version
                error_code &ec) override {
        }   // The error returned to the caller, if any

        /** Called once for each complete field in the HTTP header.

            This virtual function is invoked for each field that is received
            while parsing an HTTP message.

            @param name The known field enum value. If the name of the field
            is not recognized, this value will be @ref field::unknown.

            @param name_string The exact name of the field as received from
            the input, represented as a string.

            @param value A string holding the value of the field.

            @param ec An output parameter which the function may set to indicate
            an error. The error will be clear before this function is invoked.
        */
        void
        on_field_impl(
                field f,                    // The known-field enumeration constant
                string_view name,           // The field name string.
                string_view value,          // The field value
                error_code &ec) override   // The error returned to the caller, if any
        {
            if constexpr (isRequest)
            {
                if (f == field::unknown)
                {
                    // handle unknown fields

                    // TODO: since we already know the req_type we can simply compare a smaller subset of enum rather than checking everything
                    // TODO: s3_field_to_enum should take the req_type that is already known into account
                    auto enum_s3_field = s3_field_to_enum(name);

                    switch (enum_s3_field)
                    {
                        case x_amz_expected_bucket_owner:
                            m_parsed_struct.x_amz_expected_bucket_owner = value;
                            break;

                        case x_amz_request_payer:
                            m_parsed_struct.x_amz_request_payer = value;
                            break;
                    }

                    switch (m_parsed_struct.req_type)
                    {
                        case put_object:

                            switch (enum_s3_field)
                            {
                                case unknown:
                                    ec = http::make_error_code(boost::beast::http::error::bad_field);
                                    break;

                                case x_amz_acl:
                                    m_parsed_struct.put->x_amz_acl = value;
                                    break;

                                case x_amz_grant_full_control:
                                    m_parsed_struct.put->x_amz_grant_full_control = value;
                                    break;

                                case x_amz_grant_read:
                                    m_parsed_struct.put->x_amz_grant_read = value;
                                    break;

                                case x_amz_grant_read_acp:
                                    m_parsed_struct.put->x_amz_grant_read_acp = value;
                                    break;

                                case x_amz_grant_write_acp:
                                    m_parsed_struct.put->x_amz_grant_write_acp = value;
                                    break;

                                case x_amz_tagging:
                                    m_parsed_struct.put->x_amz_tagging = value;
                                    break;

                                case x_amz_copy_source:
                                    m_parsed_struct.req_type = copy_object;
                                    m_parsed_struct.put->copy_object->x_amz_copy_source = value;
                                    break;

                                default:
                                    ec = http::make_error_code(boost::beast::http::error::bad_field);
                                    break;

                            }
                            break;

                        case get_object:

                            switch (enum_s3_field)
                            {
                                case unknown:
                                    ec = http::make_error_code(boost::beast::http::error::bad_field);
                                    break;

                                default:
                                    ec = http::make_error_code(boost::beast::http::error::bad_field);
                                    break;

                            }
                            break;
                    }

                }
                else
                {

                    switch (f)
                    {
                        case boost::beast::http::field::host:
                            m_parsed_struct.bucket_id = value.substr(0, value.find(':'));
                            break;
                    }
                    switch (m_parsed_struct.req_type)
                    {
                        case put_object:

                            switch (f)
                            {

                                case boost::beast::http::field::content_disposition:
                                    m_parsed_struct.http_headers.content_disposition = value;
                                    break;

                                case boost::beast::http::field::content_encoding:
                                    m_parsed_struct.http_headers.content_disposition = value;
                                    break;

                                case boost::beast::http::field::content_language:
                                    m_parsed_struct.http_headers.content_language = value;
                                    break;

                                case boost::beast::http::field::content_length:
                                    m_parsed_struct.http_headers.content_length = value;
                                    break;

                                case boost::beast::http::field::content_md5:
                                    m_parsed_struct.http_headers.content_md5 = value;
                                    break;

                                case boost::beast::http::field::content_type:
                                    m_parsed_struct.http_headers.content_type = value;
                                    break;

                                case boost::beast::http::field::expires:
                                    m_parsed_struct.http_headers.expires = value;
                                    break;

                            }
                            break;

                        case get_object:

                            switch (f)
                            {

                                case boost::beast::http::field::if_match:
                                    m_parsed_struct.http_headers.if_match = value;
                                    break;

                                case boost::beast::http::field::if_modified_since:
                                    m_parsed_struct.http_headers.if_modified_since = value;
                                    break;

                                case boost::beast::http::field::if_none_match:
                                    m_parsed_struct.http_headers.if_none_match = value;
                                    break;

                                case boost::beast::http::field::if_unmodified_since:
                                    m_parsed_struct.http_headers.if_unmodified_since = value;
                                    break;

                                case boost::beast::http::field::range:
                                    m_parsed_struct.http_headers.range = value;
                                    break;

                            }
                            break;
                    }
                }
            }
        }

        /** Called once after the complete HTTP header is received.

            This virtual function is invoked once, after the complete HTTP
            header is received while parsing a message.

            @param ec An output parameter which the function may set to indicate
            an error. The error will be clear before this function is invoked.
        */
        void
        on_header_impl(
                error_code &ec) override {}   // The error returned to the caller, if any

        /** Called once before the body is processed.

            This virtual function is invoked once, before the content body is
            processed (but after the complete header is received).

            @param content_length A value representing the content length in
            bytes if the length is known (this can include a zero length).
            Otherwise, the value will be `boost::none`.

            @param ec An output parameter which the function may set to indicate
            an error. The error will be clear before this function is invoked.
        */
        void
        on_body_init_impl(
                boost::optional<
                        std::uint64_t> const &
                content_length,     // Content length if known, else `boost::none`
                error_code &ec) override {}   // The error returned to the caller, if any

        /** Called each time additional data is received representing the content body.

            This virtual function is invoked for each piece of the body which is
            received while parsing of a message. This function is only used when
            no chunked transfer encoding is present.

            @param body A string holding the additional body contents. This may
            contain nulls or unprintable characters.

            @param ec An output parameter which the function may set to indicate
            an error. The error will be clear before this function is invoked.

            @see on_chunk_body_impl
        */
        std::size_t
        on_body_impl(
                string_view s,              // A portion of the body
                error_code &ec) override
        {
            m_parsed_struct.body_stream << s;
            return s.size();
        }   // The error returned to the caller, if any

        /** Called each time a new chunk header of a chunk encoded body is received.

            This function is invoked each time a new chunk header is received.
            The function is only used when the chunked transfer encoding is present.

            @param size The size of this chunk, in bytes.

            @param extensions A string containing the entire chunk extensions.
            This may be empty, indicating no extensions are present.

            @param ec An output parameter which the function may set to indicate
            an error. The error will be clear before this function is invoked.
        */
        void
        on_chunk_header_impl(
                std::uint64_t size,         // The size of the upcoming chunk,
                // or zero for the last chunk
                string_view extension,      // The chunk extensions (may be empty)
                error_code &ec) override {}   // The error returned to the caller, if any

        /** Called each time additional data is received representing part of a body chunk.

            This virtual function is invoked for each piece of the body which is
            received while parsing of a message. This function is only used when
            no chunked transfer encoding is present.

            @param remain The number of bytes remaining in this chunk. This includes
            the contents of passed `body`. If this value is zero, then this represents
            the final chunk.

            @param body A string holding the additional body contents. This may
            contain nulls or unprintable characters.

            @param ec An output parameter which the function may set to indicate
            an error. The error will be clear before this function is invoked.

            @return This function should return the number of bytes actually consumed
            from the `body` value. Any bytes that are not consumed on this call
            will be presented in a subsequent call.

            @see on_body_impl
        */
        std::size_t
        on_chunk_body_impl(
                std::uint64_t remain,       // The number of bytes remaining in the chunk,
                // including what is being passed here.
                // or zero for the last chunk
                string_view body,           // The next piece of the chunk body
                error_code &ec) override
        {
            return 0;
        }   // The error returned to the caller, if any

        /** Called once when the complete message is received.

            This virtual function is invoked once, after successfully parsing
            a complete HTTP message.

            @param ec An output parameter which the function may set to indicate
            an error. The error will be clear before this function is invoked.
        */
        void
        on_finish_impl(
                error_code &ec) override
        {
        }   // The error returned to the caller, if any

    public:
        s3_req_object m_parsed_struct;
        std::vector<s3_fields> m_parsed_fields;

        s3_parser() = default;
    };

//------------------------------------------------------------------------------

} // namespace uh::rest

#endif // REST_NODE_SRC_S3_PARSER
