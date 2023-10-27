#include "get_bucket_request.h"

namespace uh::cluster::rest::http::model
{

    get_bucket_request::get_bucket_request(const http::request_parser<http::empty_body> & recv_req) : http_request(recv_req)
    {
        // parse and set the received request parameters
        *this = recv_req;
    }

    get_bucket_request& get_bucket_request::operator = (const http::request_parser<http::empty_body>& recv_req)
    {
        const auto& header_list = recv_req.get();

        const auto& accountId_l = header_list.find("x-amz-account-id");
        const auto& accountId_u = header_list.find("X-Amz-Account-Id");
        if (accountId_l != header_list.end() || accountId_u != header_list.end())
        {
            m_accountId = (accountId_l != header_list.end()) ? accountId_l->value() : accountId_u->value();
            m_accountIdHasBeenSet = true;
        }

        return *this;
    }

    std::map<std::string, std::string> get_bucket_request::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;
        std::stringstream ss;

        if(m_accountIdHasBeenSet)
        {
            ss << m_accountId;
            headers.emplace("x-amz-account-id",  ss.str());
            ss.str("");
        }

        return headers;
    }

} // uh::cluster::http::rest::model
