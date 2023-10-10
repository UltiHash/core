#pragma once

#include <entry_node/rest/http/http_request.h>

namespace uh::cluster::rest::http::model
{

    class abort_multi_part_upload : public rest::http::http_request
    {
    public:
        explicit abort_multi_part_upload(const http::request_parser<http::empty_body>&);

        ~abort_multi_part_upload() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::ABORT_MULTIPART_UPLOAD; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

    private:
    };

} // uh::cluster::rest::http::model
