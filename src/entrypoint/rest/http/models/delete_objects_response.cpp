#include "delete_objects_response.h"

namespace uh::cluster::rest::http::model {

delete_objects_response::delete_objects_response(const http_request& req)
    : http_response(req) {
    m_res.set(boost::beast::http::field::content_type, "application/xml");
}

delete_objects_response::delete_objects_response(
    const http_request& req, http::response<http::string_body> recv_res)
    : http_response(req, std::move(recv_res)) {
    m_res.set(boost::beast::http::field::content_type, "application/xml");
}

void delete_objects_response::add_deleted_keys(const std::string& key) {
    m_deleted.push_back(key);
}

void delete_objects_response::add_failed_keys(fail&& f) {
    m_errors.push_back(std::move(f));
}

const http::response<http::string_body>&
delete_objects_response::get_response_specific_object() {

    if (m_requestChargedHasBeenSet) {
        m_res.set("x-amz-request-charged", m_requestCharged);
    }

    std::string delete_xml_string;
    for (const auto& val : m_deleted) {
        delete_xml_string += "<Deleted>\n"
                             "<Key>" +
                             val +
                             "</Key>\n"
                             "</Deleted>\n";
    }
    for (const auto& val : m_errors) {
        auto error = error::get_code_message(val.code);
        delete_xml_string += "<Error>\n"
                             "<Key>" +
                             error.first +
                             "</Key>\n"
                             "<Code>" +
                             error.second +
                             "</Code>\n"
                             "</Error>\n";
    }

    set_body(std::string("<DeleteResult>\n" + std::move(delete_xml_string) +
                         "</DeleteResult>"));

    m_res.prepare_payload();
    return m_res;
}

} // namespace uh::cluster::rest::http::model
