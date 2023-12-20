#pragma once

#include "entrypoint/rest/http/http_request.h"

namespace uh::cluster::rest::http::model
{

    class get_bucket_request : public http_request
    {
    public:
        explicit get_bucket_request(const http::request_parser<http::empty_body>&, std::unique_ptr<rest::http::URI>);

        ~get_bucket_request() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::GET_BUCKET; }

        inline coro<void> read_body(tcp_stream& stream, boost::beast::flat_buffer& buffer) override { co_return; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

    private:

        get_bucket_request& operator = (const http::request_parser<http::empty_body>& recv_req);

        std::string m_accountId;
        bool m_accountIdHasBeenSet = false;

    };

} // uh::cluster::rest::http::model
