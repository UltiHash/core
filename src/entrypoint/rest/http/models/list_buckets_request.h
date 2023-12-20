#pragma once

#include "entrypoint/rest/http/http_request.h"

namespace uh::cluster::rest::http::model
{

    class list_buckets_request : public http_request
    {
    public:
        explicit list_buckets_request(const http::request_parser<http::empty_body>&, std::unique_ptr<rest::http::URI>);

        ~list_buckets_request() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::LIST_BUCKETS; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

        inline coro<void> read_body(tcp_stream& stream, boost::beast::flat_buffer& buffer) override { co_return; }

    private:
        list_buckets_request& operator  = (const http::request_parser<http::empty_body>& recv_req);
    };

} // uh::cluster::rest::http::model
