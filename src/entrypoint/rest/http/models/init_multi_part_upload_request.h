#pragma once

#include "entrypoint/rest/http/http_request.h"
#include "entrypoint/rest/utils/state/server_state.h"

namespace uh::cluster::rest::http::model
{

    class init_multi_part_upload_request : public rest::http::http_request
    {
    public:
        explicit init_multi_part_upload_request(const http::request_parser<http::empty_body>&, utils::server_state& , std::unique_ptr<URI>);

        ~init_multi_part_upload_request() override = default;

        [[nodiscard]] http_request_type get_request_name() const override { return http_request_type::INIT_MULTIPART_UPLOAD; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

    private:
        std::string m_bucket_name;
        std::string m_object_name;
    };

} // uh::cluster::rest::http::model
