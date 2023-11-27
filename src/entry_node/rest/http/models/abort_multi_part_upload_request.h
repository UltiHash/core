#pragma once

#include <entry_node/rest/http/http_request.h>
#include "entry_node/rest/utils/containers/internal_server_state.h"

namespace uh::cluster::rest::http::model
{

    class abort_multi_part_upload_request : public rest::http::http_request
    {
    public:
        abort_multi_part_upload_request(const http::request_parser<http::empty_body>&,
                                        utils::state&,
                                        std::unique_ptr<rest::http::URI>);

        ~abort_multi_part_upload_request() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::ABORT_MULTIPART_UPLOAD; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

    private:
        utils::state& m_internal_server_state;
        std::string m_upload_id;
        std::string m_bucket_name;
        std::string m_object_name;
    };

} // uh::cluster::rest::http::model
