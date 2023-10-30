#pragma once

#include <entry_node/rest/http/http_request.h>

namespace uh::cluster::rest::http::model
{

    class get_object_attributes_request : public http_request
    {
    public:
        explicit get_object_attributes_request(const http::request_parser<http::empty_body>&);

        ~get_object_attributes_request() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::GET_OBJECT_ATTRIBUTES; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

    private:

        get_object_attributes_request& operator  = (const http::request_parser<http::empty_body>& recv_req);

        std::string m_versionId;
        bool m_versionIdHasBeenSet = false;

        int m_maxParts;
        bool m_maxPartsHasBeenSet = false;

        int m_partNumberMarker;
        bool m_partNumberMarkerHasBeenSet = false;

        std::string m_sSECustomerAlgorithm;
        bool m_sSECustomerAlgorithmHasBeenSet = false;

        std::string m_sSECustomerKey;
        bool m_sSECustomerKeyHasBeenSet = false;

        std::string m_sSECustomerKeyMD5;
        bool m_sSECustomerKeyMD5HasBeenSet = false;

        std::string m_requestPayer;
        bool m_requestPayerHasBeenSet = false;

        std::string m_expectedBucketOwner;
        bool m_expectedBucketOwnerHasBeenSet = false;

        std::vector<std::string> m_objectAttributes;
        bool m_objectAttributesHasBeenSet = false;

    };

} // uh::cluster::rest::http::model
