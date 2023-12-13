#include "list_buckets_response.h"
#include "../../utils/time/date_time.h"

namespace uh::cluster::rest::http::model
{

    list_buckets_response::list_buckets_response(const http_request& req) : http_response(req)
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
    }

    list_buckets_response::list_buckets_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
    }

    void list_buckets_response::add_bucket(std::string bucket_to_add)
    {
        m_bucketsHasBeenSet = true;
        m_buckets.emplace_back(std::move(bucket_to_add));
    }

    const http::response<http::string_body>& list_buckets_response::get_response_specific_object()
    {

        std::string bucket_xml_string;
        if (m_bucketsHasBeenSet)
        {
            for (const auto& bucket: m_buckets)
            {
                bucket_xml_string += "      <Bucket>\n"
                                    "         <Name>" + bucket + "</Name>\n"
                                    "      </Bucket>\n";
            }
        }

        std::string owner_xml_string;
        if (m_ownerHasBeenSet)
        {
            owner_xml_string += "   <Owner>\n"
                                "      <DisplayName>" + m_owner + "</DisplayName>\n"
                                "      <ID>string</ID>\n"
                                "   </Owner>\n";
        }

        set_body(std::string("<ListAllMyBucketsResult>\n"
                             "   <Buckets>\n"
                                + bucket_xml_string +
                             "   </Buckets>\n"
                             + owner_xml_string +
                             "</ListAllMyBucketsResult>"));

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
