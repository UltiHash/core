#pragma once

#include "entrypoint/rest/http/http_request.h"
#include "entrypoint/rest/utils/state/server_state.h"

namespace uh::cluster::rest::http::model
{

    class abort_multi_part_upload_request : public rest::http::http_request
    {
    public:
        abort_multi_part_upload_request(const http::request_parser<http::empty_body>&,
                                        utils::server_state&,
                                        std::unique_ptr<rest::http::URI>);

        ~abort_multi_part_upload_request() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::ABORT_MULTIPART_UPLOAD; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

    private:
        utils::server_state& m_server_state;
        std::shared_ptr<utils::parts> m_parts_container;

        std::string m_upload_id;
        std::string m_bucket_name;
        std::string m_object_name;
    };

} // uh::cluster::rest::http::model
