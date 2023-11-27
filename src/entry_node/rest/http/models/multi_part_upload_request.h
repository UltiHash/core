#pragma once

#include <entry_node/rest/http/http_request.h>
#include "entry_node/rest/utils/containers/internal_server_state.h"

namespace uh::cluster::rest::http::model
{

    class multi_part_upload_request : public rest::http::http_request
    {
    public:
        explicit multi_part_upload_request(const http::request_parser<http::empty_body>&,
                                           utils::state&,
                                           std::unique_ptr<rest::http::URI> uri);

        ~multi_part_upload_request() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::MULTIPART_UPLOAD; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

        coro<void> read_body(tcp_stream& stream, boost::beast::flat_buffer& buffer) override;

        [[nodiscard]] const std::string& get_body() override;

        [[nodiscard]] std::size_t get_body_size() const override;

    private:

        utils::state& m_internal_server_state;
        std::shared_ptr<utils::ts_map<uint16_t, std::pair<std::string, std::string>>> m_parts_container_ptr;
        uint16_t m_part_number;
        std::string m_upload_id;
    };

} // uh::cluster::rest::http::model
