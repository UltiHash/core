#pragma once

#include <entry_node/rest/http/http_request.h>
#include <entry_node/rest/utils/containers/ts_map.h>
#include <entry_node/rest/utils/containers/ts_unordered_map.h>

namespace uh::cluster::rest::http::model
{

    class complete_multi_part_upload_request : public rest::http::http_request
    {
    public:
        complete_multi_part_upload_request(const http::request_parser<http::empty_body>&,
                                                    rest::utils::ts_unordered_map<std::string, std::shared_ptr<utils::ts_map<uint16_t, std::string>>>&,
                                                    std::string);

        ~complete_multi_part_upload_request() override;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::COMPLETE_MULTIPART_UPLOAD; }

        [[nodiscard]] std::string get_body() const override;

        [[nodiscard]] std::size_t get_body_size() const override;

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

        void clear_body() override;

    private:
        std::string m_upload_id;
        rest::utils::ts_unordered_map<std::string, std::shared_ptr<utils::ts_map<uint16_t, std::string>>>& m_uomap_multipart;
    };

} // uh::cluster::rest::http::model
