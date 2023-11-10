#pragma once

#include <entry_node/rest/http/http_request.h>
#include <entry_node/rest/utils/containers/ts_unordered_map.h>
#include <entry_node/rest/utils/containers/ts_map.h>

namespace uh::cluster::rest::http::model
{

    class abort_multi_part_upload_request : public rest::http::http_request
    {
    public:
        abort_multi_part_upload_request(const http::request_parser<http::empty_body>&,
                                        rest::utils::ts_unordered_map<std::string, std::shared_ptr<utils::ts_map<uint16_t, std::pair<std::string, std::string>>>>&,
                                        std::string,
                                        std::unique_ptr<rest::http::URI>);

        ~abort_multi_part_upload_request() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::ABORT_MULTIPART_UPLOAD; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

    private:
        rest::utils::ts_unordered_map<std::string, std::shared_ptr<utils::ts_map<uint16_t, std::pair<std::string, std::string>>>>& m_uomap_multipart;
        std::string m_upload_id;
    };

} // uh::cluster::rest::http::model
