#pragma once

#include <entry_node/rest/http/http_request.h>
#include <entry_node/rest/utils/containers/ts_map.h>
#include <entry_node/rest/utils/containers/ts_unordered_map.h>
#include "entry_node/rest/utils/containers/internal_server_state.h"

namespace uh::cluster::rest::http::model
{

    class complete_multi_part_upload_request : public rest::http::http_request
    {
    public:
        complete_multi_part_upload_request(const http::request_parser<http::empty_body>&,
                                           utils::state&,
                                           std::unique_ptr<rest::http::URI>);

        ~complete_multi_part_upload_request() override;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::COMPLETE_MULTIPART_UPLOAD; }

        [[nodiscard]] const std::string& get_body() override;

        [[nodiscard]] std::size_t get_body_size() const override;

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

        void validate_request_specific_criteria() override;

        void clear_body() override;

    private:
        void validate_request() const;

        std::shared_ptr<utils::ts_map<uint16_t, std::pair<std::string, std::string>>> m_parts_container_ptr;

        std::string m_completed_body {};
        std::string m_upload_id;
        std::string m_bucket_name;
        std::string m_object_name;
        utils::state& m_internal_server_state;
    };

} // uh::cluster::rest::http::model
