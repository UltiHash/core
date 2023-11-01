#pragma once

#include <entry_node/rest/http/http_request.h>

namespace uh::cluster::rest::http::model
{

    class delete_bucket_request : public http_request
    {
    public:
        explicit delete_bucket_request(const http::request_parser<http::empty_body>&, std::unique_ptr<rest::http::URI> uri);

        ~delete_bucket_request() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::DELETE_BUCKET; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;


    private:

        delete_bucket_request& operator = (const http::request_parser<http::empty_body>& recv_req);

        bool m_expectedBucketOwnerHasBeenSet = false;
        std::string m_expectedBucketOwner {};
    };

} // uh::cluster::rest::http::model
