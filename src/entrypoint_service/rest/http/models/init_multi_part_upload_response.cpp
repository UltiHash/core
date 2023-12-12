#include "init_multi_part_upload_response.h"

namespace uh::cluster::rest::http::model
{

    init_multi_part_upload_response::init_multi_part_upload_response(const http_request& req) : http_response(req)
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
    }

    init_multi_part_upload_response::init_multi_part_upload_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {}

    void init_multi_part_upload_response::set_upload_id(const std::string& upload_id)
    {
        m_uploadId = upload_id;
        m_uploadIdHasBeenSet = true;
    }

    const http::response<http::string_body>& init_multi_part_upload_response::get_response_specific_object()
    {

        if(m_locationHasBeenSet)
        {
            m_res.set(boost::beast::http::field::location, m_location);
        }

        if(m_requestIdHasBeenSet)
        {
            m_res.set("x-amzn-requestid", m_requestId);
        }

        set_body(std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                   "<InitiateMultipartUploadResult>\n"
                                   "<Bucket>"+ m_orig_req.get_URI().get_bucket_id() + "</Bucket>\n"
                                   "<Key>" + m_orig_req.get_URI().get_object_key() + "</Key>\n"
                                   "<UploadId>" + m_uploadId + "</UploadId>\n"
                                   "</InitiateMultipartUploadResult>"));

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
