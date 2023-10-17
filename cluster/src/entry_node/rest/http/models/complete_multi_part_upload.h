#pragma once

#include <entry_node/rest/http/http_request.h>
#include <entry_node/rest/utils/containers/ts_map.h>

namespace uh::cluster::rest::http::model
{

    class complete_multi_part_upload : public rest::http::http_request
    {
    public:
        explicit complete_multi_part_upload(const http::request_parser<http::empty_body>&,
                                            rest::utils::ts_map<uint16_t, std::string>&);

        ~complete_multi_part_upload() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::COMPLETE_MULTIPART_UPLOAD; }

        [[nodiscard]] std::string get_body() const override;

        [[nodiscard]] std::size_t get_body_size() const override;

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

    private:
        rest::utils::ts_map<uint16_t, std::string>& m_mpcontainer;
    };

} // uh::cluster::rest::http::model
