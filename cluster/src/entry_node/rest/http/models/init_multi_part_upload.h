#pragma once

#include <entry_node/rest/http/http_request.h>

namespace uh::cluster::rest::http::model
{

    class init_multi_part_upload : public rest::http::http_request
    {
    public:
        explicit init_multi_part_upload(const http::request_parser<http::empty_body>&);

        ~init_multi_part_upload() override = default;

        [[nodiscard]] inline const char * get_request_name() const override { return "InitiateMultipartUpload"; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

        coro<void> read_body(tcp_stream& stream, boost::beast::flat_buffer& buffer) override { };

    private:
    };

} // uh::cluster::rest::http::model
