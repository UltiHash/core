#include "delete_bucket_request.h"

namespace uh::cluster::rest::http::model {

delete_bucket_request::delete_bucket_request(
    const http::request_parser<http::empty_body>& recv_req,
    std::unique_ptr<rest::http::URI> uri)
    : http_request(recv_req, std::move(uri)) {
    // parse and set the received request parameters
    *this = recv_req;
}

delete_bucket_request& delete_bucket_request::operator=(
    const http::request_parser<http::empty_body>& recv_req) {
    const auto& header_list = recv_req.get();

    const auto& expec_l = header_list.find("x-amz-expected-bucket-owner");
    const auto& expec_u = header_list.find("X-Amz-Expected-Bucket-Owner");
    if (expec_l != header_list.end() || expec_u != header_list.end()) {
        m_expectedBucketOwner = (expec_l != header_list.end())
                                    ? expec_l->value()
                                    : expec_u->value();
        m_expectedBucketOwnerHasBeenSet = true;
    }

    return *this;
}

std::map<std::string, std::string>
delete_bucket_request::get_request_specific_headers() const {
    std::map<std::string, std::string> headers;
    std::stringstream ss;

    if (m_expectedBucketOwnerHasBeenSet) {
        ss << m_expectedBucketOwner;
        headers.emplace("x-amz-expected-bucket-owner", ss.str());
        ss.str("");
    }

    return headers;
}

} // namespace uh::cluster::rest::http::model
