#pragma once

#include "entrypoint/rest/http/http_request.h"
#include "entrypoint/rest/utils/state/server_state.h"

namespace uh::cluster::rest::http::model {

class complete_multi_part_upload_request : public rest::http::http_request {
public:
    complete_multi_part_upload_request(
        const http::request_parser<http::empty_body>&, utils::server_state&,
        std::unique_ptr<rest::http::URI>);

    ~complete_multi_part_upload_request() override;

    [[nodiscard]] inline http_request_type get_request_name() const override {
        return http_request_type::COMPLETE_MULTIPART_UPLOAD;
    }

    [[nodiscard]] std::map<std::string, std::string>
    get_request_specific_headers() const override;

    void validate_request_specific_criteria() override;

private:
    void validate_request() const;

    mutable std::string m_completed_body{};
    std::string m_bucket_name;
    std::string m_object_name;
    std::string m_upload_id;
    utils::server_state& m_server_state;
    constexpr static size_t MAXIMUM_CHUNK_SIZE = 5ul * 1024ul * 1024ul;
};

} // namespace uh::cluster::rest::http::model
