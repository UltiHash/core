#pragma once

#include <entry_node/rest/http/http_request.h>

namespace uh::cluster::rest::http::model
{

    class list_multi_part_uploads_request : public http_request
    {
    public:
        explicit list_multi_part_uploads_request(const http::request_parser<http::empty_body>&, std::unique_ptr<rest::http::URI>);

        ~list_multi_part_uploads_request() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::LIST_MULTI_PART_UPLOADS; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;


    private:
        list_multi_part_uploads_request& operator  = (const http::request_parser<http::empty_body>& recv_req);

        std::string m_delimiter;
        bool m_delimiterHasBeenSet = false;

        std::string m_encodingType;
        bool m_encodingTypeHasBeenSet = false;

        std::string m_keyMarker;
        bool m_keyMarkerHasBeenSet = false;

        int m_maxUploads;
        bool m_maxUploadsHasBeenSet = false;

        std::string m_prefix;
        bool m_prefixHasBeenSet = false;

        std::string m_uploadIdMarker;
        bool m_uploadIdMarkerHasBeenSet = false;

        std::string m_expectedBucketOwner;
        bool m_expectedBucketOwnerHasBeenSet = false;

        std::string m_requestPayer;
        bool m_requestPayerHasBeenSet = false;

    };

} // uh::cluster::rest::http::model
