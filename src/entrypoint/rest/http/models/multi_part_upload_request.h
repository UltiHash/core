#pragma once

#include "entrypoint/rest/http/http_request.h"
#include "entrypoint/rest/utils/state/server_state.h"

namespace uh::cluster::rest::http::model {

class multi_part_upload_request : public rest::http::http_request {
  public:
    explicit multi_part_upload_request(
        const http::request_parser<http::empty_body>&,
        std::unique_ptr<rest::http::URI> uri);

    ~multi_part_upload_request() override = default;

    [[nodiscard]] inline http_request_type get_request_name() const override {
        return http_request_type::MULTIPART_UPLOAD;
    }

    [[nodiscard]] std::map<std::string, std::string>
    get_request_specific_headers() const override;

  private:
    std::string m_upload_id;
};

} // namespace uh::cluster::rest::http::model
