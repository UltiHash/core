#include "get_bucket_response.h"

namespace uh::cluster::rest::http::model {

get_bucket_response::get_bucket_response(const http_request& req)
    : http_response(req) {
    m_res.set(boost::beast::http::field::content_type, "application/xml");
}

get_bucket_response::get_bucket_response(
    const http_request& req, http::response<http::string_body> recv_res)
    : http_response(req, std::move(recv_res)) {
    m_res.set(boost::beast::http::field::content_type, "application/xml");
}

void get_bucket_response::add_bucket(std::string bucket_to_add) {
    m_bucketHasBeenSet = true;
    m_bucket = std::move(bucket_to_add);
}

const std::string& get_bucket_response::get_bucket() const { return m_bucket; }

const http::response<http::string_body>&
get_bucket_response::get_response_specific_object() {

    std::string bucket_xml_string;
    if (m_bucketHasBeenSet) {
        bucket_xml_string += "<Bucket>" + m_bucket + "</Bucket>\n";
    }

    std::string creation_date_xml;
    if (m_creationDateHasBeenSet) {
        creation_date_xml +=
            "<CreationDate>" + m_creationDate + "</CreationDate>\n";
    }

    set_body(std::string("<GetBucketResult>\n" + bucket_xml_string +
                         creation_date_xml + "</GetBucketResult>"));

    m_res.prepare_payload();
    return m_res;
}

} // namespace uh::cluster::rest::http::model
