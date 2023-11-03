#pragma once

#include <entry_node/rest/http/http_request.h>

namespace uh::cluster::rest::http::model
{

    class get_object_request : public http_request
    {
    public:
        explicit get_object_request(const http::request_parser<http::empty_body>&, std::unique_ptr<rest::http::URI>);

        ~get_object_request() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::GET_OBJECT; }

        inline coro<void> read_body(tcp_stream& stream, boost::beast::flat_buffer& buffer) override { co_return; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

    private:

        get_object_request& operator = (const http::request_parser<http::empty_body>& recv_req);

        std::string m_bucket;
        bool m_bucketHasBeenSet = false;

        std::string m_ifMatch;
        bool m_ifMatchHasBeenSet = false;

        std::string m_ifModifiedSince;
        bool m_ifModifiedSinceHasBeenSet = false;

        std::string m_ifNoneMatch;
        bool m_ifNoneMatchHasBeenSet = false;

        std::string m_ifUnmodifiedSince;
        bool m_ifUnmodifiedSinceHasBeenSet = false;

        std::string m_key;
        bool m_keyHasBeenSet = false;

        std::string m_range;
        bool m_rangeHasBeenSet = false;

        std::string m_responseCacheControl;
        bool m_responseCacheControlHasBeenSet = false;

        std::string m_responseContentDisposition;
        bool m_responseContentDispositionHasBeenSet = false;

        std::string m_responseContentEncoding;
        bool m_responseContentEncodingHasBeenSet = false;

        std::string m_responseContentLanguage;
        bool m_responseContentLanguageHasBeenSet = false;

        std::string m_responseContentType;
        bool m_responseContentTypeHasBeenSet = false;

        std::string m_responseExpires;
        bool m_responseExpiresHasBeenSet = false;

        std::string m_versionId;
        bool m_versionIdHasBeenSet = false;

        std::string m_sSECustomerAlgorithm;
        bool m_sSECustomerAlgorithmHasBeenSet = false;

        std::string m_sSECustomerKey;
        bool m_sSECustomerKeyHasBeenSet = false;

        std::string m_sSECustomerKeyMD5;
        bool m_sSECustomerKeyMD5HasBeenSet = false;

        std::string m_requestPayer;
        bool m_requestPayerHasBeenSet = false;

        int m_partNumber;
        bool m_partNumberHasBeenSet = false;

        std::string m_expectedBucketOwner;
        bool m_expectedBucketOwnerHasBeenSet = false;

        std::string m_checksumMode;
        bool m_checksumModeHasBeenSet = false;

        std::string m_customizedAccessLogTag;
        bool m_customizedAccessLogTagHasBeenSet = false;
    };

} // uh::cluster::rest::http::model
