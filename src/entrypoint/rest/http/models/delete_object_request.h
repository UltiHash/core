#pragma once

#include "entrypoint/rest/http/http_request.h"

namespace uh::cluster::rest::http::model {

class delete_object_request : public http_request {
  public:
    explicit delete_object_request(
        const http::request_parser<http::empty_body>&,
        std::unique_ptr<rest::http::URI>);

    ~delete_object_request() override = default;

    [[nodiscard]] inline http_request_type get_request_name() const override {
        return http_request_type::DELETE_OBJECT;
    }

    [[nodiscard]] std::map<std::string, std::string>
    get_request_specific_headers() const override;

  private:
    delete_object_request&
    operator=(const http::request_parser<http::empty_body>& recv_req);

    std::string m_mFA;
    bool m_mFAHasBeenSet = false;

    std::string m_versionId;
    bool m_versionIdHasBeenSet = false;

    std::string m_requestPayer;
    bool m_requestPayerHasBeenSet = false;

    bool m_bypassGovernanceRetention;
    bool m_bypassGovernanceRetentionHasBeenSet = false;

    std::string m_expectedBucketOwner;
    bool m_expectedBucketOwnerHasBeenSet = false;
};

} // namespace uh::cluster::rest::http::model
